/*
  ==============================================================================

    JuceGranularSynth.cpp
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularSynthesizer.h"
#include "./GranularSound.h"

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
void GranularSynthesizer::setAudioBuffer(juce::AudioBuffer<float> &waveBuffer, double newFileSampleRate, size_t fileNameHash){
    assert(hasLogger());
    writeToLog(" setAudioBuffer");
    _synth_shared_state._buffer._wave_block = juce::dsp::AudioBlock<float>(waveBuffer);
    _synth_shared_state._buffer._file_sample_rate = newFileSampleRate;
    _synth_shared_state._buffer._filename_hash = fileNameHash;
}

void GranularSynthesizer::setCurrentPlaybackSampleRate(double newSampleRate) {
    _synth_shared_state._playback_sample_rate = newSampleRate;
    Synthesiser::setCurrentPlaybackSampleRate(newSampleRate);	// so far this is not necessary
}

void GranularSynthesizer::setLogger(std::function<void(const juce::String&)> loggerFunction) {
    _synth_shared_state._logger_func = std::move(loggerFunction);
}
}   // namespace nvs::gran