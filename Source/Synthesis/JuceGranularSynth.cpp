/*
  ==============================================================================

    JuceGranularSynth.cpp
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "JuceGranularSynth.h"

GranularVoice::GranularVoice(double const &sampleRate,  std::span<float> const &wavespan, double const &fileSampleRate, size_t nGrains)
:	granularSynthGuts(sampleRate, wavespan,
					  fileSampleRate, nGrains)
{}
void GranularVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition)
{
	
}
void GranularVoice::stopNote (float velocity, bool allowTailOff)
{
	
}

void GranularVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples)
{
	
}
void GranularVoice::pitchWheelMoved (int newPitchWheelValue) {}
void GranularVoice::controllerMoved (int controllerNumber, int newControllerValue) {}
bool GranularVoice::canPlaySound (juce::SynthesiserSound *)
{
	return true;
}



GranularSynthesizer::GranularSynthesizer(double const &sampleRate,
					std::span<float> const &wavespan, double const &fileSampleRate,
					size_t nGrains)
{
	for (int i = 0; i < numVoices; ++i){
		std::unique_ptr<GranularVoice> voice = std::make_unique<GranularVoice>(sampleRate, wavespan, fileSampleRate, nGrains);
		addVoice(voice.get());
	}
}

