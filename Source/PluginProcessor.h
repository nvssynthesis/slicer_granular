/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GranularSynthesis.h"
#include "dsp_util.h"
#include "params.h"
#include "DataStructures.h"
#include "frozen/map.h"

//==============================================================================
/**
*/
class Slicer_granularAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
	//==============================================================================
	Slicer_granularAudioProcessor();
	~Slicer_granularAudioProcessor() override;
	
	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;
	
#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
	
	void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
	
	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;
	
	//==============================================================================
	const juce::String getName() const override;
	
	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;
	
	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram (int index) override;
	const juce::String getProgramName (int index) override;
	void changeProgramName (int index, const juce::String& newName) override;
	
	//==============================================================================
	void getStateInformation (juce::MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;
	
	juce::AudioProcessorValueTreeState apvts;
	
	void writeToLog(std::string const s);
	void loadAudioFile(juce::File const f);
	
	bool triggerValFromEditor {false};
#if 0
	class EditorInformant{
		float loudness {0.f};
		float loudnessThreshold {0.f};
	public:
		void reset(){
			loudness = 0.f;
		}
		void accumulate(float v){
			loudness += v*v;
		}
		void updateEndOfBlock(){
			//sharableValue = loudness;
		}
		float query(){
			return 0;//sharableLoudness;
		}
	};
//	EditorInformant editorInformant;
#endif
	template<typename T>
	struct editorInformant{
		T val;
	};
	editorInformant<float> rmsInformant;
	editorInformant<float> rmsWAinformant;
	
private:
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	juce::AudioFormatManager formatManager;
	
	RMS<float> rms;
	WeightedAveragingBuffer<float, 3> weightAvg;

	class AudioBuffersChannels{
	private:
		typedef std::array<std::vector<float>, 2> stereoVector_t;
		
		std::array<stereoVector_t, 2> stereoBuffers;
		std::span<float> activeSpan;
		
		unsigned int activeBufferIdx { 0 };
		inline void updateActiveBufferIndex(){
			activeBufferIdx = !activeBufferIdx;
			jassert((activeBufferIdx == 0) | (activeBufferIdx == 1));
		}

	public:
		AudioBuffersChannels(){
			activeSpan = std::span<float>(stereoBuffers[activeBufferIdx][0]);
		}
		std::span<float> const &getActiveSpanRef(){
			return activeSpan;
		}
		[[nodiscard]]
		std::array<float *const, 2> prepareForWrite(size_t length, size_t numChannels){
			auto inactiveBufferIdx = !activeBufferIdx;
			
			numChannels = std::min(numChannels, stereoBuffers[inactiveBufferIdx].size());
			for (int i = 0; i < numChannels; ++i){
				stereoBuffers[inactiveBufferIdx][i].resize(length);
			}
			std::array<float *const, 2> ptrsToWriteTo = {
				stereoBuffers[inactiveBufferIdx][0].data(),
				stereoBuffers[inactiveBufferIdx][1].data()
			};
			return ptrsToWriteTo;
		}

		inline void updateActive(){
			updateActiveBufferIndex();
			activeSpan = std::span<float>(stereoBuffers[activeBufferIdx][0]);
		}

	};
	AudioBuffersChannels audioBuffersChannels;
	
#pragma message("get these statically from default map in params.h")
	float lastTranspose 	{getParamDefault(params_e::transpose)};
	float lastPosition 		{0.f};
	float lastSpeed 		{getParamDefault(params_e::speed)};
	float lastDuration 		{100.f};
	float lastSkew 			{0.5f};
	float lastPan 			{0.5f};

	float lastTransposeRand {0.f};
	float lastPositionRand 	{0.f};
	float lastSpeedRand 	{0.f};
	float lastDurationRand 	{0.f};
	float lastSkewRand 		{0.f};
	float lastPanRand 		{0.f};
	
#define STATIC_MAP 0
#define FROZEN_MAP 1
	static_assert(!(STATIC_MAP && FROZEN_MAP));	// one or the other, or neither, but not both
	
#if STATIC_MAP
	#define MAP StaticMap
#elif FROZEN_MAP
	#define MAP frozen::map
#endif
	
#if (STATIC_MAP | FROZEN_MAP)
	using granMembrSetFunc = void(nvs::gran::genGranPoly1::*) (float);

	static constexpr inline MAP<params_e, granMembrSetFunc, static_cast<size_t>(params_e::count)>
	paramSetterMap {
		std::make_pair<params_e, granMembrSetFunc>(params_e::transpose, 		&nvs::gran::genGranPoly1::setTranspose),
		std::make_pair<params_e, granMembrSetFunc>(params_e::position, 			&nvs::gran::genGranPoly1::setPosition),
		std::make_pair<params_e, granMembrSetFunc>(params_e::speed, 			&nvs::gran::genGranPoly1::setSpeed),
		std::make_pair<params_e, granMembrSetFunc>(params_e::duration, 			&nvs::gran::genGranPoly1::setDuration),
		std::make_pair<params_e, granMembrSetFunc>(params_e::skew, 				&nvs::gran::genGranPoly1::setSkew),
		std::make_pair<params_e, granMembrSetFunc>(params_e::pan, 				&nvs::gran::genGranPoly1::setPan),
		std::make_pair<params_e, granMembrSetFunc>(params_e::transp_randomness, &nvs::gran::genGranPoly1::setTransposeRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::pos_randomness, 	&nvs::gran::genGranPoly1::setPositionRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::speed_randomness, 	&nvs::gran::genGranPoly1::setSpeedRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::dur_randomness, 	&nvs::gran::genGranPoly1::setDurationRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::skew_randomness, 	&nvs::gran::genGranPoly1::setSkewRandomness),
		std::make_pair<params_e, granMembrSetFunc>(params_e::pan_randomness, 	&nvs::gran::genGranPoly1::setPanRandomness)
	};
	template <auto Start, auto End>
	constexpr void paramSet();
	
	MAP<params_e, float *, static_cast<size_t>(params_e::count)>
	lastParamsMap{
		std::make_pair<params_e, float *>(params_e::transpose, 	&lastTranspose),
		std::make_pair<params_e, float *>(params_e::position, 	&lastPosition),
		std::make_pair<params_e, float *>(params_e::speed, 		&lastSpeed),
		std::make_pair<params_e, float *>(params_e::duration, 	&lastDuration),
		std::make_pair<params_e, float *>(params_e::skew, 		&lastSkew),
		std::make_pair<params_e, float *>(params_e::pan, 		&lastPan),
		std::make_pair<params_e, float *>(params_e::transp_randomness, 	&lastTransposeRand),
		std::make_pair<params_e, float *>(params_e::pos_randomness, 	&lastPositionRand),
		std::make_pair<params_e, float *>(params_e::speed_randomness, 	&lastSpeedRand),
		std::make_pair<params_e, float *>(params_e::dur_randomness, 	&lastDurationRand),
		std::make_pair<params_e, float *>(params_e::skew_randomness, 	&lastSkewRand),
		std::make_pair<params_e, float *>(params_e::pan_randomness, 	&lastPanRand)
	};
#endif
	float lastSampleRate 	{ 0.f };
	int lastSamplesPerBlock { 0 };
	
#if USING_ESSENTIA
	// loads into vector<Real> via func in OnsetAnalysis
	nvs::ess::EssentiaInitializer ess_init;
	nvs::ess::EssentiaHolder ess_hold;
#endif
	nvs::gran::genGranPoly1 gen_granular;

	// button to recompute analysis
	
	// sliders to change analysis settings
	
	//...
	
	// granular synth
	
	//======logging=======================
	juce::File logFile;
	juce::FileLogger fileLogger;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessor)
};
