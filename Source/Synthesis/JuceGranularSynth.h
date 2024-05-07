/*
  ==============================================================================

    JuceGranularSynth.h
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "GranularSynthesis.h"
#include "../params.h"

#ifndef FROZEN_MAP
#define FROZEN_MAP 1
#endif

#if FROZEN_MAP
	#include "frozen/map.h"
	#define STATIC_MAP 0
	#define MAP frozen::map
#endif

#if STATIC_MAP
	#include "DataStructures.h"
	#define MAP StaticMap
#endif

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
	GranularVoice(double const &sampleRate,  std::span<float> const &wavespan, double const &fileSampleRate, unsigned long seed = 1234567890UL);
	void prepareToPlay(double sampleRate, int samplesPerBlock);	// why not override??
	
	void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
	void stopNote (float velocity, bool allowTailOff) override;
	bool isVoiceActive() const override;
	
	void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
	void pitchWheelMoved (int newPitchWheelValue) override;
	void controllerMoved (int controllerNumber, int newControllerValue) override;
	bool canPlaySound (juce::SynthesiserSound *) override ;
	
	template <auto Start, auto End>
	constexpr void granularMainParamSet(juce::AudioProcessorValueTreeState &apvts) {
		float tmp;

		if constexpr (Start < End){
			constexpr params_e p = static_cast<params_e>(Start);
			tmp = *apvts.getRawParameterValue(getParamElement<p, param_elem_e::name>());
			float *last = lastGranularMainParamsMap.at(p);
			if (tmp != *last){
				*last = tmp;
				granMembrSetFunc setFunc = paramSetterMap.at(p);
				(granularSynthGuts->*setFunc)(tmp);	// could replace with std::invoke
			}
			
			granularMainParamSet<Start + 1, End>(apvts);
		}
	}
	template <auto Start, auto End>
	constexpr void envelopeParamSet(juce::AudioProcessorValueTreeState &apvts) {
		float tmp;
		if constexpr (Start < End){
			constexpr params_e p = static_cast<params_e>(Start);
			tmp = *apvts.getRawParameterValue(getParamElement<p, param_elem_e::name>());
			float *last = lastEnvelopeParamsMap.at(p);
			if (tmp != *last){
				*last = tmp;
				juceVoiceSetFunc setFunc = envParamSetterMap.at(p);
				(this->*setFunc)(tmp);	// could replace with std::invoke
			}
			
			envelopeParamSet<Start + 1, End>(apvts);
		}
	}
	void setAmpAttack(float newAttack);
	void setAmpDecay(float newDecay);
	void setAmpSustain(float newSustain);
	void setAmpRelease(float newRelease);
private:
	nvs::gran::genGranPoly1 *granularSynthGuts;
	int lastMidiNoteNumber {0};
	juce::ADSR adsr;
	juce::ADSR::Parameters adsrParameters {0.1, 0.3, 0.5, 0.05};

	
	float lastTranspose 	{getParamDefault(params_e::transpose)};
	float lastPosition 		{getParamDefault(params_e::position)};
	float lastSpeed 		{getParamDefault(params_e::speed)};
	float lastDuration 		{getParamDefault(params_e::duration)};
	float lastSkew 			{getParamDefault(params_e::skew)};
	float lastPlateau 		{getParamDefault(params_e::plateau)};
	float lastPan 			{getParamDefault(params_e::pan)};

	float lastTransposeRand {getParamDefault(params_e::transp_randomness)};
	float lastPositionRand 	{getParamDefault(params_e::pos_randomness)};
	float lastSpeedRand 	{getParamDefault(params_e::speed_randomness)};
	float lastDurationRand 	{getParamDefault(params_e::dur_randomness)};
	float lastSkewRand 		{getParamDefault(params_e::skew_randomness)};
	float lastPlateauRand	{getParamDefault(params_e::plat_randomness)};
	float lastPanRand 		{getParamDefault(params_e::pan_randomness)};

	float lastAmpAttack 	{getParamDefault(params_e::amp_attack)};
	float lastAmpDecay 		{getParamDefault(params_e::amp_decay)};
	float lastAmpSustain 	{getParamDefault(params_e::amp_sustain)};
	float lastAmpRelease 	{getParamDefault(params_e::amp_release)};

#if (STATIC_MAP | FROZEN_MAP)
	using granMembrSetFunc = void(nvs::gran::genGranPoly1::*) (float);

	static constexpr inline MAP<params_e, granMembrSetFunc, static_cast<size_t>(params_e::count_main_granular_params)>
	paramSetterMap {
		std::make_pair<params_e, granMembrSetFunc>(params_e::transpose, 		&nvs::gran::genGranPoly1::setTranspose),
		std::make_pair<params_e, granMembrSetFunc>(params_e::position, 			&nvs::gran::genGranPoly1::setPosition),
		std::make_pair<params_e, granMembrSetFunc>(params_e::speed, 			&nvs::gran::genGranPoly1::setSpeed),
		std::make_pair<params_e, granMembrSetFunc>(params_e::duration, 			&nvs::gran::genGranPoly1::setDuration),
		std::make_pair<params_e, granMembrSetFunc>(params_e::skew, 				&nvs::gran::genGranPoly1::setSkew),
		std::make_pair<params_e, granMembrSetFunc>(params_e::plateau,			&nvs::gran::genGranPoly1::setPlateau),
		std::make_pair<params_e, granMembrSetFunc>(params_e::pan, 				&nvs::gran::genGranPoly1::setPan),
		std::make_pair<params_e, granMembrSetFunc>(params_e::transp_randomness, &nvs::gran::genGranPoly1::setTransposeRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::pos_randomness, 	&nvs::gran::genGranPoly1::setPositionRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::speed_randomness, 	&nvs::gran::genGranPoly1::setSpeedRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::dur_randomness, 	&nvs::gran::genGranPoly1::setDurationRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::skew_randomness, 	&nvs::gran::genGranPoly1::setSkewRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::plat_randomness,	&nvs::gran::genGranPoly1::setPlateauRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::pan_randomness, 	&nvs::gran::genGranPoly1::setPanRandomness),
	};
	using juceVoiceSetFunc = void(GranularVoice::*) (float);
	static constexpr inline MAP<params_e, juceVoiceSetFunc, 4> envParamSetterMap {
		std::make_pair<params_e, juceVoiceSetFunc>(params_e::amp_attack, &GranularVoice::setAmpAttack),
		std::make_pair<params_e, juceVoiceSetFunc>(params_e::amp_decay, &GranularVoice::setAmpDecay),
		std::make_pair<params_e, juceVoiceSetFunc>(params_e::amp_sustain, &GranularVoice::setAmpSustain),
		std::make_pair<params_e, juceVoiceSetFunc>(params_e::amp_release, &GranularVoice::setAmpRelease),
	};
	/*
	 this map is unnecessary because these pointed-to floats are never called by name. could just use an std::array<float, static_cast<size_t>(params_e::count)> lastParams
	 */
	MAP<params_e, float *, static_cast<size_t>(params_e::count_main_granular_params)>
	lastGranularMainParamsMap{
		std::make_pair<params_e, float *>(params_e::transpose, 	&lastTranspose),
		std::make_pair<params_e, float *>(params_e::position, 	&lastPosition),
		std::make_pair<params_e, float *>(params_e::speed, 		&lastSpeed),
		std::make_pair<params_e, float *>(params_e::duration, 	&lastDuration),
		std::make_pair<params_e, float *>(params_e::skew, 		&lastSkew),
		std::make_pair<params_e, float *>(params_e::plateau,	&lastPlateau),
		std::make_pair<params_e, float *>(params_e::pan, 		&lastPan),
		std::make_pair<params_e, float *>(params_e::transp_randomness, 	&lastTransposeRand),
		std::make_pair<params_e, float *>(params_e::pos_randomness, 	&lastPositionRand),
		std::make_pair<params_e, float *>(params_e::speed_randomness, 	&lastSpeedRand),
		std::make_pair<params_e, float *>(params_e::dur_randomness, 	&lastDurationRand),
		std::make_pair<params_e, float *>(params_e::skew_randomness, 	&lastSkewRand),
		std::make_pair<params_e, float *>(params_e::plat_randomness, 	&lastPlateauRand),
		std::make_pair<params_e, float *>(params_e::pan_randomness, 	&lastPanRand)
	};
	MAP<params_e, float *, 4>
	lastEnvelopeParamsMap{
		std::make_pair<params_e, float *>(params_e::amp_attack,		&lastAmpAttack),
		std::make_pair<params_e, float *>(params_e::amp_decay,		&lastAmpDecay),
		std::make_pair<params_e, float *>(params_e::amp_sustain,	&lastAmpSustain),
		std::make_pair<params_e, float *>(params_e::amp_release,	&lastAmpRelease)
	};
#endif
	
	struct dbg_counter {
		using int_t = unsigned long;
		int_t i {0};
		bool go(int_t cmp){
			if (++i == cmp){
				i = 0;
				return true;
			} else {
				return false;
			}
		}
	} counter;
};


class GranularSynthesizer	:	public juce::Synthesiser
{
public:
	GranularSynthesizer(double const &sampleRate,
						std::span<float> const &wavespan, double const &fileSampleRate,
						unsigned int num_voices);
		
	template <auto Start, auto End>
	constexpr void granularMainParamSet(juce::AudioProcessorValueTreeState &apvts){

		if constexpr (Start < End){
			
			juce::SynthesiserVoice* voice = getVoice(Start);
			if (GranularVoice* granularVoice = dynamic_cast<GranularVoice*>(voice)){
				granularVoice->granularMainParamSet<0, static_cast<int>(params_e::count_main_granular_params)>(apvts);
			}
			else {
				jassert (false);
			}
			granularMainParamSet<Start + 1, End>(apvts);
		}
	}
	
	template <auto Start, auto End>
	constexpr void envelopeParamSet(juce::AudioProcessorValueTreeState &apvts){
		if constexpr (Start < End){
			juce::SynthesiserVoice* voice = getVoice(Start);
			if (GranularVoice* granularVoice = dynamic_cast<GranularVoice*>(voice)){
				granularVoice->envelopeParamSet<static_cast<int>(params_e::count_main_granular_params)+1,
												static_cast<int>(params_e::count_envelope_params)>
												(apvts);
			}
			else {
				jassert (false);
			}
			envelopeParamSet<Start + 1, End>(apvts);
		}
	}
private:
	unsigned int numVoices {0};
};
