/*
  ==============================================================================

    GranularVoice.cpp
    Created: 7 May 2024 11:04:44am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularVoice.h"

GranularVoice::GranularVoice(nvs::gran::GranularSynthSharedState  *const synth_shared_state, unsigned long seed, int voice_id)
:	_synth_shared_state(synth_shared_state)
,	_voice_shared_state {
		._gaussian_rng {seed},
		._expo_rng {seed + 123456789UL},
		._voice_id = voice_id
	}
{}

template<>
void GranularVoice::initSynthGuts<nvs::gran::PolyGrain>() {
	granularSynthGuts = std::make_unique<nvs::gran::PolyGrain>(_synth_shared_state, &_voice_shared_state);
	granularSynthGuts->setReadBounds({0.0, 1.0});
}

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
	(void)sound;
	
	int velIntegral = static_cast<int>(velocity * 127.f);
//	_voice_shared_state.trigger = 1.0;

	if (adsr.isActive()) {}
	else {
		granularSynthGuts->clearNotes();
		granularSynthGuts->noteOn(midiNoteNumber, velIntegral);
	}
	
	{
		auto &apvts = _synth_shared_state->_apvts;
		adsr.setParameters(juce::ADSR::Parameters (
			(float)*apvts.getRawParameterValue("amp_env_attack"),
			(float)*apvts.getRawParameterValue("amp_env_decay"),
			(float)*apvts.getRawParameterValue("amp_env_sustain"),
			(float)*apvts.getRawParameterValue("amp_env_release")
		));
	}
	
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
std::vector<nvs::gran::GrainDescription> GranularVoice::getGrainDescriptions() const {
	return _grainDescriptions;
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
	granularSynthGuts->setParams();

	auto totalNumOutputChannels = outputBuffer.getNumChannels();

	double envelope;
	for (auto samp = startSample; samp < startSample + numSamples; ++samp){
		envelope = adsr.getNextSample();
		if (envelope != envelope) {
			logger("ENVELOPE has NaN");
		}
		envelope *= envelope;
		
		std::array<float, 2> output = (*granularSynthGuts)(0.f /*_voice_shared_state.trigger*/);
//		_voice_shared_state.trigger = 0.f;
		
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
bool GranularVoice::canPlaySound (juce::SynthesiserSound *)
{
	return true;
}
