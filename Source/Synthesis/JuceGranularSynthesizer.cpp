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
					std::span<float> const &wavespan, double const &fileSampleRate,
					unsigned int num_voices)
{
	numVoices = num_voices;
	clearVoices();
	unsigned long seed = 1234567890UL;
	for (int i = 0; i < numVoices; ++i) {
		auto voice = new GranularVoice(std::make_unique<nvs::gran::genGranPoly1>(sampleRate, wavespan, fileSampleRate, seed));
		addVoice(voice);
		++seed;
	}
	
	clearSounds();
	auto sound = new GranularSound;
	addSound(sound);
}
