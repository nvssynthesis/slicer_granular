/** TODO:
	-output gain
*/
#pragma once

#include <JuceHeader.h>
#include "Synthesis/GranularSynthesis.h"
#include "Synthesis/JuceGranularSynthesizer.h"
#include "dsp_util.h"
#include "misc_util.h"
#include "params.h"

//==============================================================================

class Slicer_granularAudioProcessor  : 	public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
	//==============================================================================
	Slicer_granularAudioProcessor();
	
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
	
	void writeToLog(juce::String const &s);
	void loadAudioFile(juce::File const f, bool notifyEditor);
	
	void loadAudioFilesFolder(juce::File const folder);

	juce::String getSampleFilePath() const;
	juce::AudioFormatManager &getAudioFormatManager();
	juce::AudioProcessorValueTreeState &getAPVTS();

	void writeGrainDescriptionData(const std::vector<nvs::gran::GrainDescription> &newData);
	void readGrainDescriptionData(std::vector<nvs::gran::GrainDescription> &outData);
	
	// change broadcasters
	void addSampleManagementGutsListener(juce::ChangeListener *newListener){
		sampleManagementGuts.addChangeListener(newListener);
	}
	void addMeasuredGrainDescriptionsListener(juce::ChangeListener *newListener){
		measuredGrainDescriptions.addChangeListener(newListener);
	}
	void removeSampleManagementGutsListener(juce::ChangeListener *newListener){
		sampleManagementGuts.removeChangeListener(newListener);
	}
	void removeMeasuredGrainDescriptionsListener(juce::ChangeListener *newListener){
		measuredGrainDescriptions.removeChangeListener(newListener);
	}
private:
	nvs::util::LoggingGuts loggingGuts;
	
	void readInAudioFileToBuffer(juce::File const f);
	void loadAudioFileAsync(juce::File const file, bool notifyEditor);

	nvs::util::SampleManagementGuts sampleManagementGuts;
	
	juce::AudioProcessorValueTreeState apvts;
	juce::SpinLock audioBlockLock;
	
	GranularSynthesizer granular_synth_juce;
	
	nvs::util::MeasuredData measuredGrainDescriptions;
	
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessor)
};
