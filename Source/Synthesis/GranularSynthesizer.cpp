/*
  ==============================================================================

    JuceGranularSynth.cpp
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularSynthesizer.h"
#include "./GranularSound.h"
#include "../utils/dsp_util.h"

namespace nvs::gran {
GranularSynthesizer::GranularSynthesizer(juce::AudioProcessorValueTreeState &apvts)
:	_synth_shared_state(apvts)
{
    GranularSynthesizer::setCurrentPlaybackSampleRate(44100.0);	// setting to some default rate, because we need a sensible sample rate (not 0) to construct the voices, to avoid divide by zero
    initializeVoices();
    clearSounds();
    addSound(new GranularSound);
    setNoteStealingEnabled(true);
    setPositionAlignmentSetting(PositionAlignmentSetting::alignAtWindowPeak);
}
void GranularSynthesizer::initializeVoices() {
    clearVoices();
    unsigned long seed = 1234567890UL;
    totalNumGrains_ = 0;
    for (int i = 0; i < num_voices; ++i) {
        const auto voice = GranularVoice::create<nvs::gran::PolyGrain>(&_synth_shared_state, seed, i);
        addVoice(voice);
        totalNumGrains_ += GranularVoice::getNumGrains();
        ++seed;
    }
    // previously, addSound occurred here
    setCurrentPlaybackSampleRate(getSampleRate());	// if voices' sample rates need updating, this shall do it
}
std::vector<nvs::gran::GrainDescription> GranularSynthesizer::getGrainDescriptions() const {
    std::vector<nvs::gran::GrainDescription> grainDescriptions;
    grainDescriptions.reserve(totalNumGrains_);

    for (const auto &v : voices) {
        if (GranularVoice const *const gv = dynamic_cast<GranularVoice *const>(v)){
            std::vector<nvs::gran::GrainDescription> const theseDescriptions = gv->getGrainDescriptions();
            for (const auto &desc : theseDescriptions) {
                grainDescriptions.push_back(desc);
            }
        }
        else {
            jassert(false);
        }
    }
    return grainDescriptions;
}
void GranularSynthesizer::setAudioBuffer(juce::AudioBuffer<float> &waveBuffer, const double newFileSampleRate, const juce::int64 audioHash){
    assert(hasLogger());
    writeToLog(" setAudioBuffer");
    auto &[_wave_block, _loudness_profile, _file_sample_rate, _audio_hash] = _synth_shared_state._buffer;

    _wave_block = juce::dsp::AudioBlock<float>(waveBuffer);
    _file_sample_rate = newFileSampleRate;
    _audio_hash = audioHash;
    util::calculateSymmetricEnvelope(_wave_block, _loudness_profile, newFileSampleRate);
}

void GranularSynthesizer::setCurrentPlaybackSampleRate(double newSampleRate) {
    _synth_shared_state._playback_sample_rate = newSampleRate;
    Synthesiser::setCurrentPlaybackSampleRate(newSampleRate);	// so far this is not necessary
}

void GranularSynthesizer::prepareToPlay(const double sampleRate, const int samplesPerBlock) {
    setCurrentPlaybackSampleRate(sampleRate);

    _synth_shared_state._reverb_send_buffer.setSize(2, samplesPerBlock, false, true, true);

    _reverb.setSampleRate(sampleRate);
    // fixed for now (non-variable, per-grain send amount is the only thing that varies);
    // wetLevel/dryLevel are set so the reverb outputs pure wet -- "dry" is already the
    // unprocessed voice output, mixed in separately.
    _reverb.setParameters(juce::Reverb::Parameters {
        .roomSize = 0.5f,
        .damping = 0.5f,
        .wetLevel = 1.0f,
        .dryLevel = 0.0f,
        .width = 1.0f,
        .freezeMode = 0.0f
    });
}

void GranularSynthesizer::renderVoices(juce::AudioBuffer<float> &outputAudio, const int startSample, const int numSamples) {
    auto &sendBuffer = _synth_shared_state._reverb_send_buffer;
    jassert (startSample + numSamples <= sendBuffer.getNumSamples());
    sendBuffer.clear(startSample, numSamples);

    Synthesiser::renderVoices(outputAudio, startSample, numSamples);	// voices accumulate dry into outputAudio, wet into sendBuffer

    auto *wetL = sendBuffer.getWritePointer(0, startSample);
    auto *wetR = sendBuffer.getWritePointer(1, startSample);
    _reverb.processStereo(wetL, wetR, numSamples);

    outputAudio.addFrom(0, startSample, wetL, numSamples);
    outputAudio.addFrom(1, startSample, wetR, numSamples);
}

void GranularSynthesizer::setLogger(std::function<void(const juce::String&)> loggerFunction) {
    _synth_shared_state._logger_func = std::move(loggerFunction);
}
}   // namespace nvs::gran