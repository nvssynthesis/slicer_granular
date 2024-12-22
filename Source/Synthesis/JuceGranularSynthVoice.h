/*
  ==============================================================================

    JuceGranularSynthVoice.h
    Created: 7 May 2024 11:04:44am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once

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
#include "./GranularSynthesis.h"
#include "../params.h"


class GranularVoice	:	public juce::SynthesiserVoice
{
public:
	GranularVoice(std::unique_ptr<nvs::gran::genGranPoly1> synthGuts)
	:	granularSynthGuts{std::move(synthGuts)}
	{}
	void setAudioBlock(juce::dsp::AudioBlock<float> audioBlock, double fileSampleRate);
	void setCurrentPlaybackSampleRate(double sampleRate) override;

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
				(granularSynthGuts.get()->*setFunc)(tmp);	// could replace with std::invoke
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
	
	nvs::gran::genGranPoly1* getGranularSynthGuts(){
		return granularSynthGuts.get();
	}
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
private:
	std::unique_ptr<nvs::gran::genGranPoly1> granularSynthGuts;
	int lastMidiNoteNumber {0};
	juce::ADSR adsr;
	juce::ADSR::Parameters adsrParameters {0.1, 0.3, 0.5, 0.05};
	
	
	std::function<void(const juce::String&)> logger = nullptr;

	
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
