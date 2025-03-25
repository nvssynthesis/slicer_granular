/*
  ==============================================================================

    JuceGranularSynthVoice.cpp
    Created: 7 May 2024 11:04:44am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "JuceGranularSynthVoice.h"


//void GranularVoice::setAudioBlock(juce::dsp::AudioBlock<float> audioBlock, double fileSampleRate){
//	granularSynthGuts->setAudioBlock(audioBlock, fileSampleRate);
//}
void GranularVoice::setLogger(std::function<void(const juce::String&)> loggerFunction)
{
	logger = loggerFunction;
	granularSynthGuts->setLogger(loggerFunction);
}
void GranularVoice::setCurrentPlaybackSampleRate(double sampleRate){
	adsr.reset();
	adsr.setSampleRate(sampleRate);
	granularSynthGuts->setSampleRate(sampleRate);
	this->SynthesiserVoice::setCurrentPlaybackSampleRate(sampleRate);
}
void GranularVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	juce::ignoreUnused(samplesPerBlock);
	setCurrentPlaybackSampleRate(sampleRate);
}
void GranularVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition)
{
	int velIntegral = static_cast<int>(velocity * 127.f);
	
	if (adsr.isActive()){
//		trigger = 1.f;
	}
	else {
		granularSynthGuts->clearNotes();
		granularSynthGuts->noteOn(midiNoteNumber, velIntegral);
	}
	
	adsr.setParameters(adsrParameters);
	
	lastMidiNoteNumber = midiNoteNumber;
	adsr.noteOn();
}
void GranularVoice::stopNote (float velocity, bool allowTailOff)
{
	(void)velocity;
	if (allowTailOff)	// releasing regularly
	{
		adsr.noteOff();
	}
	else	// !allowTailOff, so voice was stolen
	{
		adsr.reset();
		clearCurrentNote();
		granularSynthGuts->clearNotes();
	}
}
bool GranularVoice::isVoiceActive() const {
	return adsr.isActive();
}

void GranularVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples)
{
	if (!isVoiceActive()){
		granularSynthGuts->clearNotes();
		granularSynthGuts->noteOff(lastMidiNoteNumber);
		granularSynthGuts->setGrainsIdle();
		
		_grainDescriptions = granularSynthGuts->getGrainDescriptions();
		for (auto &gd : _grainDescriptions) {
			gd.window = 0.f;
		}
		return;
	}
	
	float trigger = 0.f;
	auto totalNumOutputChannels = outputBuffer.getNumChannels();

	double envelope;
	for (auto samp = startSample; samp < startSample + numSamples; ++samp){
		envelope = adsr.getNextSample();
		if (envelope != envelope) {
			logger("ENVELOPE has NaN");
		}
		std::array<float, 2> output = (*granularSynthGuts)(trigger);
//		trigger = 0.f;
		output[0] *= envelope;
		output[1] *= envelope;
		for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
			auto* channelData = outputBuffer.getWritePointer (channel);
			*(channelData + samp) += output[channel];
		}
	}
	// query grains for descriptions
	_grainDescriptions = granularSynthGuts->getGrainDescriptions();
	for (auto &gd : _grainDescriptions) {
		gd.window *= envelope;
	}
}
void GranularVoice::pitchWheelMoved (int newPitchWheelValue) {
	// apply pitch wheel
}
void GranularVoice::controllerMoved (int controllerNumber, int newControllerValue) {
	// apply (CC aspects of) modulation matrix?
}
void GranularVoice::setAmpAttack(float newAttack){
	adsrParameters.attack = newAttack;
}
void GranularVoice::setAmpDecay(float newDecay){
	adsrParameters.decay = newDecay;
}
void GranularVoice::setAmpSustain(float newSustain){
	adsrParameters.sustain = newSustain;
}
void GranularVoice::setAmpRelease(float newRelease){
	adsrParameters.release = newRelease;
}
bool GranularVoice::canPlaySound (juce::SynthesiserSound *)
{
	return true;
}
