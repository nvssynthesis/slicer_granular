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

GranularSynthesizer::GranularSynthesizer(double const &sampleRate,
					juce::AudioBuffer<float> &waveBuffer, double const &fileSampleRate,
					unsigned int num_voices)
{
	clearVoices();
	unsigned long seed = 1234567890UL;
	for (int i = 0; i < num_voices; ++i) {

		auto voice = new GranularVoice(std::make_unique<nvs::gran::genGranPoly1>(sampleRate, waveBuffer, fileSampleRate, seed));
		addVoice(voice);
		++seed;
	}
	
	clearSounds();
	auto sound = new GranularSound;
	addSound(sound);
}
void GranularSynthesizer::setAudioBlock(juce::AudioBuffer<float> &waveBuffer){
	logger("GranularSynthesizer: setAudioBlock");
	for (int i = 0; i < getNumVoices(); i++)
	{
		if (auto voice = dynamic_cast<GranularVoice*>(getVoice(i)))
		{
			voice->setAudioBlock(waveBuffer);
		}
	}
}
void GranularSynthesizer::setLogger(std::function<void(const juce::String&)> loggerFunction)
{
	logger = loggerFunction;
	for (int i = 0; i < getNumVoices(); i++){
		if (auto voice = dynamic_cast<GranularVoice*>(getVoice(i))){
			voice->setLogger(loggerFunction);
		}
	}
}
