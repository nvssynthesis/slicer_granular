/*
  ==============================================================================

    GranularVoice.cpp
    Created: 7 May 2024 11:04:44am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularVoice.h"

namespace nvs::gran {
GranularVoice::GranularVoice(GranularSynthSharedState  *const synth_shared_state, const unsigned long seed, const int voice_id)
:	_synth_shared_state(synth_shared_state)
,	_voice_shared_state {
    ._gaussian_rng {BoxMuller(seed)},
    ._voice_id = voice_id
}
{}

void GranularVoice::setLogger(const std::function<void(const String&)>& loggerFunction)
{
    logger = loggerFunction;
    granularSynthGuts->setLogger(loggerFunction);
}
void GranularVoice::setCurrentPlaybackSampleRate(const double sampleRate){
    adsr.reset();
    adsr.setSampleRate(sampleRate);
    granularSynthGuts->setSampleRate(sampleRate);
    this->SynthesiserVoice::setCurrentPlaybackSampleRate(sampleRate);
}
void GranularVoice::prepareToPlay(const double sampleRate, int samplesPerBlock)
{
    ignoreUnused(samplesPerBlock);
    setCurrentPlaybackSampleRate(sampleRate);
}
void GranularVoice::startNote (const int midiNoteNumber, const float velocity, SynthesiserSound *sound, int currentPitchWheelPosition)
{
    (void)sound;

    const int velIntegral = static_cast<int>(velocity * 127.f);
    //	_voice_shared_state.trigger = 1.0;

    if (isVoiceActive()) {}
    else {
        granularSynthGuts->clearNotes();
        granularSynthGuts->noteOn(midiNoteNumber, velIntegral);
        _voice_shared_state.forceGrainTrigger = true;
    }

    const auto &apvts = _synth_shared_state->_apvts;

    // the mode is decided here, once, for this note's whole lifetime -- a mode switch made
    // while this note is sounding will not affect it, only notes triggered after the switch.
    noteUsesBreakpointEnv = static_cast<int>(*apvts.getRawParameterValue("amp_env_mode")) != 0;

    if (noteUsesBreakpointEnv) {
        {
            const SpinLock::ScopedTryLockType lock(_synth_shared_state->_amp_breakpoint_env_lock);
            if (lock.isLocked()) {
                cachedBreakpointShape = _synth_shared_state->_amp_breakpoint_env_shape;
            }
            // else: try-lock lost to a concurrent GUI edit; fall back to the last shape this voice saw.
        }
        const double lengthSeconds = *apvts.getRawParameterValue("amp_env_bp_length");
        breakpointRuntimeDesc = toRuntimeDescriptor(cachedBreakpointShape, lengthSeconds, getSampleRate());
        breakpointReleaseTriggered = false;
        breakpointHolding = false;
        breakpointEnvActive = !breakpointRuntimeDesc.empty();
        if (breakpointEnvActive) {
            sustainSegIndexForThisNote = jlimit(0, static_cast<int>(breakpointRuntimeDesc.size()) - 1, cachedBreakpointShape.sustainIndex);
            breakpointEnv.reset(&breakpointRuntimeDesc);
        }
    }
    else {
        adsr.setParameters(ADSR::Parameters (
            *apvts.getRawParameterValue("amp_env_attack"),
            *apvts.getRawParameterValue("amp_env_decay"),
            *apvts.getRawParameterValue("amp_env_sustain"),
            *apvts.getRawParameterValue("amp_env_release")
        ));
        adsr.noteOn();
    }

    lastMidiNoteNumber = midiNoteNumber;
}
void GranularVoice::stopNote (const float velocity, const bool allowTailOff)
{
    (void)velocity;
    if (allowTailOff)	// releasing regularly
    {
        if (noteUsesBreakpointEnv) {
            if (breakpointEnvActive) {
                if (!breakpointHolding) {
                    // released before reaching the sustain point (e.g. a very short note):
                    // jump from wherever the envelope currently sits straight into the release
                    // portion, skipping the rest of the attack/decay.
                    const int numSegments = static_cast<int>(breakpointRuntimeDesc.size());
                    const int releaseIndex = jlimit(0, jmax(0, numSegments - 1), sustainSegIndexForThisNote);
                    breakpointEnv.advanceToSegment(releaseIndex);
                }
                // else: already frozen exactly at the start of the release portion -- just
                // un-freeze below and let it play from here.
                breakpointHolding = false;
                breakpointReleaseTriggered = true;
            }
        } else {
            adsr.noteOff();
        }
    }
    else	// !allowTailOff, so voice was stolen
    {
        adsr.reset();
        breakpointEnvActive = false;
        clearCurrentNote();
        granularSynthGuts->clearNotes();
    }
}
bool GranularVoice::isVoiceActive() const {
    return noteUsesBreakpointEnv ? breakpointEnvActive : adsr.isActive();
}
std::vector<GrainDescription> GranularVoice::getGrainDescriptions() const {
    return _grainDescriptions;
}
void GranularVoice::renderNextBlock (AudioBuffer< float > &outputBuffer, const int startSample, const int numSamples)
{
    if (!isVoiceActive()){
        granularSynthGuts->clearNotes();
        granularSynthGuts->noteOff(lastMidiNoteNumber);
        granularSynthGuts->setGrainsIdle();

        _grainDescriptions = granularSynthGuts->getGrainDescriptions();
        for (auto &gd : _grainDescriptions) {
            gd.window = 0.f;
        }
        return;
    }
    granularSynthGuts->setParams();

    const auto totalNumOutputChannels = outputBuffer.getNumChannels();

    const auto envelopeVal = [startSample, numSamples, totalNumOutputChannels, this, &outputBuffer] -> float {
        float env {1.0};
        for (auto samp = startSample; samp < startSample + numSamples; ++samp){
            if (noteUsesBreakpointEnv) {
                if (breakpointEnvActive) {
                    if (breakpointHolding) {
                        // frozen at the sustain point until note-off (see stopNote)
                        env = breakpointRuntimeDesc[sustainSegIndexForThisNote].initialValue;
                    } else if (!breakpointReleaseTriggered
                               && breakpointEnv.getCurrentSegmentIndex() == sustainSegIndexForThisNote) {
                        // just reached the sustain point: freeze here without consuming any of
                        // the release segment's samples yet.
                        breakpointHolding = true;
                        env = breakpointRuntimeDesc[sustainSegIndexForThisNote].initialValue;
                    } else if (breakpointEnv.getSample(env) && breakpointReleaseTriggered) {
                        // never reads past a just-finished release (which would otherwise
                        // auto-loop back to the start of the shape) -- see
                        // MultiSegmentEnvelopeGenerator::getSample.
                        breakpointEnvActive = false;
                    }
                } else {
                    env = 0.f;
                }
            } else {
                env = adsr.getNextSample();
            }
            if (env != env) { logger("ENVELOPE has NaN"); }
            env *= env;

            DryWet output = granularSynthGuts->doProcess(0.f /*_voice_shared_state.trigger*/);
            //		_voice_shared_state.trigger = 0.f;

            output.dry[0] *= env;
            output.dry[1] *= env;
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
                auto* channelData = outputBuffer.getWritePointer (channel);
                *(channelData + samp) += output.dry[channel];
            }

            // send this voice's per-sample reverb-send contribution into the shared buss; the single
            // reverb owned by GranularSynthesizer reads/processes this buffer once per block.
            auto &sendBuffer = _synth_shared_state->_reverb_send_buffer;
            jassert (samp < sendBuffer.getNumSamples());
            sendBuffer.addSample(0, samp, output.wet[0] * env);
            sendBuffer.addSample(1, samp, output.wet[1] * env);
        }
        return env;
    }();
    // query grains for descriptions
    _grainDescriptions = granularSynthGuts->getGrainDescriptions();
    for (auto &gd : _grainDescriptions) {
        gd.window *= envelopeVal;
    }
}
void GranularVoice::pitchWheelMoved (int newPitchWheelValue) {
    // apply pitch wheel
}
void GranularVoice::controllerMoved (int controllerNumber, int newControllerValue) {
    // apply (CC aspects of) modulation matrix?
}
bool GranularVoice::canPlaySound (SynthesiserSound *)
{
    return true;
}
}   // namespace nvs::gran