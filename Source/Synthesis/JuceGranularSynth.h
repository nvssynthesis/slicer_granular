/*
  ==============================================================================

    JuceGranularSynth.h
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "GranularSynthesis.h"
#include <JuceHeader.h>

class GranularSound	:	public juce::SynthesiserSound
{
public:
	bool appliesToNote (int midiNoteNumber) override;
	bool appliesToChannel (int midiChannel) override;
};

class GranularVoice	:	public juce::SynthesiserVoice
{
public:
	GranularVoice(double const &sampleRate,  std::span<float> const &wavespan, double const &fileSampleRate, size_t nGrains);
	void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
	void stopNote (float velocity, bool allowTailOff) override;

	void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
	void pitchWheelMoved (int newPitchWheelValue) override;
	void controllerMoved (int controllerNumber, int newControllerValue) override;
	bool canPlaySound (juce::SynthesiserSound *) override ;
private:
	nvs::gran::genGranPoly1 granularSynthGuts;
	int lastMidiNoteNumber {0};
};


class GranularSynthesizer	:	public juce::Synthesiser
{
public:
	GranularSynthesizer(double const &sampleRate,
						std::span<float> const &wavespan, double const &fileSampleRate,
						size_t nGrains);
private:
	int numVoices = 2;
};
