/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GranularSynthesis.h"

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
	
private:
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	juce::AudioFormatManager formatManager;
	
	class AudioBuffersChannels{
	private:
		typedef std::array<std::vector<float>, 2> stereoVector;
		typedef std::array<float *, 2> stereoVectorData;
		
		std::array<stereoVector, 2> buffers;
		std::array<stereoVectorData, 2> dataPointers;
		
		unsigned int activeBufferIdx { 0 };
		inline void updateActiveBufferIndex(){
			activeBufferIdx = !activeBufferIdx;
			jassert((activeBufferIdx == 0) | (activeBufferIdx == 1));
		}
		inline void updateDataPointers(){
			dataPointers[activeBufferIdx][0] = buffers[activeBufferIdx][0].data();
			dataPointers[activeBufferIdx][1] = buffers[activeBufferIdx][1].data();
		}
	public:
		void prepareForWrite(size_t length, size_t numChannels){
			updateActiveBufferIndex();
			numChannels = std::min(numChannels, buffers[activeBufferIdx].size());
			for (int i = 0; i < numChannels; ++i){
				buffers[activeBufferIdx][i].resize(length);
			}
			updateDataPointers();
		}
		stereoVectorData &getActiveStereoDataPtrs(){
			return dataPointers[activeBufferIdx];
		}
		stereoVector &getActiveStereoVector(){
			return buffers[activeBufferIdx];
		}
	};
	AudioBuffersChannels audioBuffersChannels;
	
	float lastPosition {0.f};
	float lastSpeed {0.1f};
	float lastDuration {100.f};
	float lastSkew {0.5f};
	
	float lastPan {0.5f};

	float lastPositionRand {0.f};
	float lastSpeedRand {0.f};
	float lastDurationRand {0.f};
	float lastSkewRand {0.f};
	float lastPanRand {0.f};

	
	float lastSampleRate 	{ 0.f };
	int lastSamplesPerBlock { 0 };
	
	// button to load audio file
	
	// loads into vector<Real> via func in OnsetAnalysis
	nvs::ess::EssentiaInitializer ess_init;
	nvs::ess::EssentiaHolder ess_hold;
	
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
