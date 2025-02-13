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

GranularSynthesizer::GranularSynthesizer()
{
	setCurrentPlaybackSampleRate(44100.0);	// setting to some default rate, because we need a sensible sample rate (not 0) to construct the voices, to avoid divide by zero
	initializeVoices();
	clearSounds();
	auto sound = new GranularSound;
	addSound(sound);
	setNoteStealingEnabled(true);
}
std::vector<nvs::gran::GrainDescription> GranularSynthesizer::getGrainDescriptions() const {
	std::vector<nvs::gran::GrainDescription> grainDescriptions;
	grainDescriptions.reserve(totalNumGrains_);

	for (const auto &v : voices) {
		if (GranularVoice const * const gv = dynamic_cast<GranularVoice * const>(v)){
			std::vector<nvs::gran::GrainDescription> const theseDescriptions = gv->getGrainDescriptions();
			grainDescriptions.insert(grainDescriptions.end(), theseDescriptions.begin(), theseDescriptions.end());
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

		auto voice = new GranularVoice(std::make_unique<nvs::gran::genGranPoly1>(seed));
		addVoice(voice);
		totalNumGrains_ += voice->getNumGrains();
		++seed;
	}
	// previously, addSound occured here
	setVoicesAudioBlock();	// if audio block needs to be passed down, this shall do it
	setCurrentPlaybackSampleRate(getSampleRate());	// if voices' sample rates need updating, this shall do it
}
void GranularSynthesizer::setAudioBlock(juce::AudioBuffer<float> &waveBuffer, double newFileSampleRate){
	assert(logger_);
	logger_("GranularSynthesizer: setAudioBlock");
	audioBlock_ = juce::dsp::AudioBlock<float>(waveBuffer);
	fileSampleRate_ = newFileSampleRate;
	
	setVoicesAudioBlock();
}

void GranularSynthesizer::setCurrentPlaybackSampleRate(double sampleRate){
	for (auto &v : voices){
		v->setCurrentPlaybackSampleRate(sampleRate);
	}
	Synthesiser::setCurrentPlaybackSampleRate(sampleRate);
}

void GranularSynthesizer::setLogger(std::function<void(const juce::String&)> loggerFunction)
{
	logger_ = loggerFunction;
	for (int i = 0; i < num_voices; i++){
		if (auto voice = dynamic_cast<GranularVoice*>(getVoice(i))){
			voice->setLogger(loggerFunction);
		}
	}
}
void GranularSynthesizer::setVoicesAudioBlock(){
	for (int i = 0; i < num_voices; i++)
	{
		if (auto voice = dynamic_cast<GranularVoice*>(getVoice(i)))
		{
			voice->setAudioBlock(audioBlock_, fileSampleRate_);
		}
	}
}
