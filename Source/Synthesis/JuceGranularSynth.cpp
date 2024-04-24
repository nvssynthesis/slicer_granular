/*
  ==============================================================================

    JuceGranularSynth.cpp
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "JuceGranularSynth.h"

bool GranularSound::appliesToNote (int midiNoteNumber) {return true;}
bool GranularSound::appliesToChannel (int midiChannel) {return true;}


GranularVoice::GranularVoice(double const &sampleRate,  std::span<float> const &wavespan, double const &fileSampleRate, size_t nGrains)
:	granularSynthGuts(sampleRate, wavespan,
					  fileSampleRate, nGrains)
{}
void GranularVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition)
{
	int velIntegral = static_cast<int>(velocity * 127.f);
	granularSynthGuts.noteOn(midiNoteNumber, velIntegral);
	lastMidiNoteNumber = midiNoteNumber;
}
void GranularVoice::stopNote (float velocity, bool allowTailOff)
{
	(void)velocity;
	(void)allowTailOff;
	granularSynthGuts.noteOff(lastMidiNoteNumber);
}

void GranularVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples)
{
	float trigger = 0.f;
	auto totalNumOutputChannels = outputBuffer.getNumChannels();
	for (auto samp = 0; samp < outputBuffer.getNumSamples(); ++samp){
		std::array<float, 2> output = granularSynthGuts(trigger);
		for (int channel = 0; channel < totalNumOutputChannels; ++channel)
		{
			auto* channelData = outputBuffer.getWritePointer (channel);
			*(channelData + samp) = output[channel];
		}
	}
}
void GranularVoice::pitchWheelMoved (int newPitchWheelValue) {
	// apply pitch wheel
}
void GranularVoice::controllerMoved (int controllerNumber, int newControllerValue) {
	// apply modulation matrix?
}
bool GranularVoice::canPlaySound (juce::SynthesiserSound *)
{
	return true;
}

GranularSynthesizer::GranularSynthesizer(double const &sampleRate,
					std::span<float> const &wavespan, double const &fileSampleRate,
					size_t nGrains)
{
	for (int i = 0; i < numVoices; ++i) {
		auto voice = std::make_unique<GranularVoice>(sampleRate, wavespan, fileSampleRate, nGrains);
		addVoice(voice.get());
	}
}

