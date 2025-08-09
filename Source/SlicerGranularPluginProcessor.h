/** TODO:
	-output gain
*/
#pragma once

#include <JuceHeader.h>
#include "Synthesis/GranularSynthesis.h"
#include "Synthesis/GranularSynthesizer.h"
#include "dsp_util.h"
#include "misc_util.h"
#include "Params/params.h"
#include "Service/PresetManager.h"

//==============================================================================

class SlicerGranularAudioProcessor  : 	public juce::AudioProcessor
,										public juce::ChangeListener
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
	//==============================================================================
	template<typename GranularAudioProcessor_t>
	static GranularAudioProcessor_t *create() {
		auto obj = new GranularAudioProcessor_t();
		obj->initialize();
		return obj;
	}
	~SlicerGranularAudioProcessor();
	
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
	void changeListenerCallback (juce::ChangeBroadcaster *source) override;
	//==============================================================================
	void writeToLog(juce::String const &s);
	virtual void loadAudioFile(juce::File const f, bool notifyEditor);

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
	int getCurrentWaveSize() {
		return sampleManagementGuts.getLength();
	}
	nvs::service::PresetManager &getPresetManager() { return presetManager; }
	
	nvs::gran::GranularSynthSharedState const &viewSynthSharedState() const;
protected:
	SlicerGranularAudioProcessor();
	void initialize() {
		initSynth();
		_granularSynth->setLogger([this](const juce::String& message)
		{
			if (juce::FileLogger::getCurrentLogger()){
				loggingGuts.fileLogger.logMessage(message);
			}
		});
	}
	virtual void initSynth(){
		// this one-line function gets overriden by TSNGranularAudioProcessor to create a derived type of synthesizer
		_granularSynth = std::make_unique<nvs::gran::GranularSynthesizer>(apvts);
	}
	
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
	nvs::service::PresetManager presetManager;

	std::unique_ptr<nvs::gran::GranularSynthesizer> _granularSynth;
	
	juce::SpinLock audioBlockLock;
	void readInAudioFileToBuffer(const juce::File &f);
	
private:
	nvs::util::MeasuredData measuredGrainDescriptions;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlicerGranularAudioProcessor)
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
