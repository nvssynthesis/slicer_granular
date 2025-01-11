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
	struct LoggingGuts {
		LoggingGuts();
		~LoggingGuts();
		juce::File logFile;
		juce::FileLogger fileLogger;
		void logIfNaNOrInf(juce::AudioBuffer<float> buffer);
	};
	LoggingGuts loggingGuts;
	
	void readInAudioFileToBuffer(juce::File const f);
	void loadAudioFileAsync(juce::File const file, bool notifyEditor);
public:
	struct SampleManagementGuts : public juce::ChangeBroadcaster
	{
		SampleManagementGuts();
		~SampleManagementGuts();
		juce::AudioFormatManager formatManager;
		const juce::String audioFilePathValueTreeStateIdentifier {"sampleFilePath"};
		juce::String sampleFilePath;
		juce::AudioBuffer<float> sampleBuffer;
		double lastFileSampleRate { 0.0 };
		float normalizationValue { 1.f };	// a MULTIPLIER for the overall output, based on the inverse of the absolute max value for the current sample
	};
private:
	SampleManagementGuts sampleManagementGuts;
	
	juce::AudioProcessorValueTreeState apvts;
	juce::SpinLock audioBlockLock;
	
	GranularSynthesizer granular_synth_juce;
	
public:
	struct MeasuredData : public juce::ChangeBroadcaster
	{
		std::vector<nvs::gran::GrainDescription> data0;
		std::vector<nvs::gran::GrainDescription> data1;
		std::atomic<bool> dataReady {false};
		std::atomic<int> activeBufferIdx {0};
	};
private:
	MeasuredData measuredGrainDescriptions;
	
	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessor)
};

class AudioFileLoaderThread : public juce::Thread
{
public:
	AudioFileLoaderThread(Slicer_granularAudioProcessor& processor, juce::File fileToLoad, bool notify)
		: juce::Thread("AudioFileLoader"),
		  audioProcessor(processor),
		  file(fileToLoad),
		  notifyEditor(notify) {}

	void run() override {
		// Perform the file loading operation
		audioProcessor.loadAudioFile(file, notifyEditor);
	}

private:
	Slicer_granularAudioProcessor& audioProcessor;
	juce::File file;
	bool notifyEditor;
};
