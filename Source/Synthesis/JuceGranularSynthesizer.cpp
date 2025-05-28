/*
  ==============================================================================

    JuceGranularSynth.cpp
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "JuceGranularSynthesizer.h"
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include <fmt/core.h>
#endif
#include "./JuceGranularSynthSound.h"

GranularSynthesizer::GranularSynthesizer(juce::AudioProcessorValueTreeState &apvts)
:	_synth_shared_state(apvts)
{
	setCurrentPlaybackSampleRate(44100.0);	// setting to some default rate, because we need a sensible sample rate (not 0) to construct the voices, to avoid divide by zero
	initializeVoices();
	clearSounds();
	addSound(new GranularSound);
	setNoteStealingEnabled(true);
	setPositionAlignmentSetting(PositionAlignmentSetting::alignAtWindowPeak);
}
std::vector<nvs::gran::GrainDescription> GranularSynthesizer::getGrainDescriptions() const {
	std::vector<nvs::gran::GrainDescription> grainDescriptions;
	grainDescriptions.reserve(totalNumGrains_);

	for (const auto &v : voices) {
		if (GranularVoice const * const gv = dynamic_cast<GranularVoice * const>(v)){
			std::vector<nvs::gran::GrainDescription> const theseDescriptions = gv->getGrainDescriptions();
			for (const auto &desc : theseDescriptions) {
				grainDescriptions.push_back(desc);
			}
		}
		else {
			assert(false);
		}
	}
	return grainDescriptions;
}
void GranularSynthesizer::initializeVoices() {
	clearVoices();
	unsigned long seed = 1234567890UL;
	totalNumGrains_ = 0;
	for (int i = 0; i < num_voices; ++i) {
		auto voice = new GranularVoice(std::make_unique<nvs::gran::PolyGrain>(&_synth_shared_state, i, seed));
		addVoice(voice);
		totalNumGrains_ += voice->getNumGrains();
		++seed;
	}
	// previously, addSound occured here
	setCurrentPlaybackSampleRate(getSampleRate());	// if voices' sample rates need updating, this shall do it
}
void GranularSynthesizer::setAudioBlock(juce::AudioBuffer<float> &waveBuffer, double newFileSampleRate, size_t fileNameHash){
	assert(hasLogger());
	writeToLog(" setAudioBlock");
	_synth_shared_state._buffer._wave_block = juce::dsp::AudioBlock<float>(waveBuffer);
	_synth_shared_state._buffer._file_sample_rate = newFileSampleRate;
	_synth_shared_state._buffer._filename_hash = fileNameHash;
}

void GranularSynthesizer::setCurrentPlaybackSampleRate(double newSampleRate) {
	_synth_shared_state._playback_sample_rate = newSampleRate;
	Synthesiser::setCurrentPlaybackSampleRate(newSampleRate);	// so far this is not necessary
}

void GranularSynthesizer::setLogger(std::function<void(const juce::String&)> loggerFunction) {
	_synth_shared_state._logger_func = loggerFunction;
}
