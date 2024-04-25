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
	GranularVoice(double const &sampleRate,  std::span<float> const &wavespan, double const &fileSampleRate);
	void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
	void stopNote (float velocity, bool allowTailOff) override;

	void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
	void pitchWheelMoved (int newPitchWheelValue) override;
	void controllerMoved (int controllerNumber, int newControllerValue) override;
	bool canPlaySound (juce::SynthesiserSound *) override ;
	
	template <auto Start, auto End>
	constexpr void paramSet(juce::AudioProcessorValueTreeState &apvts) {
		float tmp;

		if constexpr (Start < End){
			constexpr params_e p = static_cast<params_e>(Start);
			tmp = *apvts.getRawParameterValue(getParamElement<p, param_elem_e::name>());
			float *last = lastParamsMap.at(p);
			if (tmp != *last){
				*last = tmp;
				granMembrSetFunc setFunc = paramSetterMap.at(p);
				(granularSynthGuts.*setFunc)(tmp);	// could replace with std::invoke
			}
			
			paramSet<Start + 1, End>(apvts);
		}
	}
private:
	nvs::gran::genGranPoly1 granularSynthGuts;
	int lastMidiNoteNumber {0};
	
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
	
#if (STATIC_MAP | FROZEN_MAP)
	using granMembrSetFunc = void(nvs::gran::genGranPoly1::*) (float);

	static constexpr inline MAP<params_e, granMembrSetFunc, static_cast<size_t>(params_e::count)>
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
		std::make_pair<params_e, granMembrSetFunc>(params_e::pan_randomness, 	&nvs::gran::genGranPoly1::setPanRandomness)
	};
	
	/*
	 this map is unnecessary because these pointed-to floats are never called by name. could just use an std::array<float, static_cast<size_t>(params_e::count)> lastParams
	 */
	MAP<params_e, float *, static_cast<size_t>(params_e::count)>
	lastParamsMap{
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
#endif
};


class GranularSynthesizer	:	public juce::Synthesiser
{
public:
	GranularSynthesizer(double const &sampleRate,
						std::span<float> const &wavespan, double const &fileSampleRate,
						unsigned int num_voices);
	
	
	
	template <auto Start, auto End>
	constexpr void paramSet(juce::AudioProcessorValueTreeState &apvts){

		if constexpr (Start < End){
			
			juce::SynthesiserVoice* voice = getVoice(Start);
			if (GranularVoice* granularVoice = dynamic_cast<GranularVoice*>(voice)){
				granularVoice->paramSet<0, static_cast<int>(params_e::count)>(apvts);
			}
			else {
				jassert (false);
			}
			paramSet<Start + 1, End>(apvts);
		}
	}
private:
	unsigned int numVoices {0};
};
