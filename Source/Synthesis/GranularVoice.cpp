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

    if (adsr.isActive()) {}
    else {
        granularSynthGuts->clearNotes();
        granularSynthGuts->noteOn(midiNoteNumber, velIntegral);
    }

    {
        const auto &apvts = _synth_shared_state->_apvts;
        adsr.setParameters(ADSR::Parameters (
            *apvts.getRawParameterValue("amp_env_attack"),
            *apvts.getRawParameterValue("amp_env_decay"),
            *apvts.getRawParameterValue("amp_env_sustain"),
            *apvts.getRawParameterValue("amp_env_release")
        ));
    }

    lastMidiNoteNumber = midiNoteNumber;
    adsr.noteOn();
}
void GranularVoice::stopNote (const float velocity, const bool allowTailOff)
{
    (void)velocity;
    if (allowTailOff)	// releasing regularly
    {
        adsr.noteOff();
    }
    else	// !allowTailOff, so voice was stolen
    {
        adsr.reset();
        clearCurrentNote();
        granularSynthGuts->clearNotes();
    }
}
bool GranularVoice::isVoiceActive() const {
    return adsr.isActive();
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
            env = adsr.getNextSample();
            if (env != env) { logger("ENVELOPE has NaN"); }
            env *= env;

            std::array<float, 2> output = (*granularSynthGuts)(0.f /*_voice_shared_state.trigger*/);
            //		_voice_shared_state.trigger = 0.f;

            output[0] *= env;
            output[1] *= env;
            for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
                auto* channelData = outputBuffer.getWritePointer (channel);
                *(channelData + samp) += output[channel];
            }
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