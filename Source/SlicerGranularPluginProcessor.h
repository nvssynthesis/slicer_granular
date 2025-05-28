/** TODO:
	-output gain
*/
#pragma once

#include <JuceHeader.h>
#include "Synthesis/GranularSynthesis.h"
#include "Synthesis/JuceGranularSynthesizer.h"
#include "dsp_util.h"
#include "misc_util.h"
#include "Params/params.h"

//==============================================================================

class SlicerGranularAudioProcessor  : 	public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
	//==============================================================================
	SlicerGranularAudioProcessor();
	
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
	//==============================================================================
	void writeToLog(juce::String const &s);
	virtual void loadAudioFile(juce::File const f, bool notifyEditor);

	juce::String getSampleFilePath() const;
	juce::AudioFormatManager &getAudioFormatManager();
	juce::AudioProcessorValueTreeState &getAPVTS();
	juce::ValueTree &getNonAutomatableState();
	
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
	int getCurrentWaveSize() {
		return sampleManagementGuts.sampleBuffer.getNumSamples();
	}

	nvs::gran::GranularSynthSharedState const &viewSynthSharedState();
protected:
	nvs::util::SampleManagementGuts sampleManagementGuts;
	nvs::util::LoggingGuts loggingGuts;
	
	juce::int64 lastLogTimeMs = 0;
	void logRateLimited(const juce::String& message, int cooldownMs)
	{
		auto now = juce::Time::getMillisecondCounter();
		if (now - lastLogTimeMs >= cooldownMs)
		{
			lastLogTimeMs = now;
			writeToLog(message);
		}
	}
	
	juce::AudioProcessorValueTreeState apvts;
	juce::ValueTree nonAutomatableState;
	
	std::unique_ptr<GranularSynthesizer> _granularSynth;
	
	juce::SpinLock audioBlockLock;
	void readInAudioFileToBuffer(juce::File const f);
private:
	void loadAudioFileAsync(juce::File const file, bool notifyEditor);
	
	nvs::util::MeasuredData measuredGrainDescriptions;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlicerGranularAudioProcessor)
};

class AudioFileLoaderThread : public juce::Thread
{
public:
	AudioFileLoaderThread(SlicerGranularAudioProcessor& processor, juce::File fileToLoad, bool notify)
		: juce::Thread("AudioFileLoader"),
		  audioProcessor(processor),
		  file(fileToLoad),
		  notifyEditor(notify) {}

	void run() override {
		// Perform the file loading operation
		audioProcessor.loadAudioFile(file, notifyEditor);
	}

private:
	SlicerGranularAudioProcessor& audioProcessor;
	juce::File file;
	bool notifyEditor;
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
