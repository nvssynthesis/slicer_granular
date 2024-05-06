/*
  ==============================================================================

    JuceGranularSynth.cpp
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "JuceGranularSynth.h"
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include <fmt/core.h>
#endif

bool GranularSound::appliesToNote (int midiNoteNumber) {return true;}
bool GranularSound::appliesToChannel (int midiChannel) {return true;}


GranularVoice::GranularVoice(double const &sampleRate,  std::span<float> const &wavespan, double const &fileSampleRate, unsigned long seed)
:	granularSynthGuts(sampleRate, wavespan, fileSampleRate, seed)
{}

void GranularVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
 {
	adsr.reset();
	adsr.setSampleRate(sampleRate);
}

void GranularVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition)
{
	int velIntegral = static_cast<int>(velocity * 127.f);
	
	if (adsr.isActive()){}
	else {
		granularSynthGuts.noteOn(midiNoteNumber, velIntegral);
	}
	
	adsr.setParameters(adsrParameters);
	
	lastMidiNoteNumber = midiNoteNumber;
	adsr.noteOn();
}
void GranularVoice::stopNote (float velocity, bool allowTailOff)
{
	(void)velocity;
	granularSynthGuts.noteOff(lastMidiNoteNumber);
	if (allowTailOff)	// releasing regularly
	{
		adsr.noteOff();
	}
	else	// !allowTailOff, so voice was stolen
	{
		adsr.reset();
		clearCurrentNote();
	}
}
bool GranularVoice::isVoiceActive() const {
	return adsr.isActive();
}


void GranularVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples)
{
	float trigger = 0.f;
	auto totalNumOutputChannels = outputBuffer.getNumChannels();

	for (auto samp = startSample; samp < startSample + numSamples; ++samp){
		double envelope = adsr.getNextSample();
		std::array<float, 2> output = granularSynthGuts(trigger);
//		output[0] *= envelope;
//		output[1] *= envelope;
		for (int channel = 0; channel < totalNumOutputChannels; ++channel)
		{
			auto* channelData = outputBuffer.getWritePointer (channel);
			*(channelData + samp) += output[channel];
		}
	}
	if (!isVoiceActive()){
		granularSynthGuts.clearNotes();
	}
}
void GranularVoice::pitchWheelMoved (int newPitchWheelValue) {
	// apply pitch wheel
}
void GranularVoice::controllerMoved (int controllerNumber, int newControllerValue) {
	// apply (CC aspects of) modulation matrix?
}
bool GranularVoice::canPlaySound (juce::SynthesiserSound *)
{
	return true;
}

GranularSynthesizer::GranularSynthesizer(double const &sampleRate,
					std::span<float> const &wavespan, double const &fileSampleRate,
					unsigned int num_voices)
{
	numVoices = num_voices;
	clearVoices();
	unsigned long seed = 1234567890UL;
	for (int i = 0; i < numVoices; ++i) {
		auto voice = new GranularVoice(sampleRate, wavespan, fileSampleRate, seed);
		addVoice(voice);
		++seed;
	}
	
	clearSounds();
	auto sound = new GranularSound;
	addSound(sound);
}

//void GranularSynthesizer::setCurrentPlaybackSampleRate(double sampleRate) {
//	for (auto &voice : voices){
//		voice->setCurrentPlaybackSampleRate(sampleRate);
//	}
//}
