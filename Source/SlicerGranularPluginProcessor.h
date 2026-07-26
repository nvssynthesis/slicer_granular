/** TODO:
	-output gain
*/
#pragma once

#include <JuceHeader.h>

#include "AnalysisUsing.h"
#include "Synthesis/GranularSynthesis.h"
#include "Synthesis/GranularSynthesizer.h"
#include "utils/misc_util_juce.h"
#include "SampleManager.h"
#include "Service/PresetManager.h"

//==============================================================================


class SlicerGranularAudioProcessor  : 	public AudioProcessor
,										public ChangeListener
                            #if JucePlugin_Enable_ARA
                             , public AudioProcessorARAExtension
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
	~SlicerGranularAudioProcessor() override;
	
	//==============================================================================
	void prepareToPlay (double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;
	
#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

	void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
	
	//==============================================================================
	AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;
	
	//==============================================================================
	const String getName() const override;
	
	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;
	
	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram (int index) override;
	const String getProgramName (int index) override;
	void changeProgramName (int index, const String& newName) override;
	
	//==============================================================================
	void getStateInformation (MemoryBlock& destData) override;
	void setStateInformation (const void* data, int sizeInBytes) override;
	//==============================================================================
	void changeListenerCallback (ChangeBroadcaster *source) override;
	//==============================================================================
	void writeToLog(String const &s);
	void loadStoredAudioFileAndUpdateState();	// calls loadAudioFileAndUpdateState using path stored in APVTS
	virtual void loadAudioFileAndUpdateState(File f, bool notifyEditor);

	String getSampleFilePath() const;
	String getAudioHash() const;
	AudioFormatManager &getAudioFormatManager();
	AudioProcessorValueTreeState &getAPVTS();
	
	void writeGrainDescriptionData(const std::vector<nvs::gran::GrainDescription> &newData);
	void readGrainDescriptionData(std::vector<nvs::gran::GrainDescription> &outData);
	
	// change broadcasters
	void addSampleManagementGutsListener(ChangeListener *newListener){
		sampleManager.addChangeListener(newListener);
	}
	void addMeasuredGrainDescriptionsListener(ChangeListener *newListener){
		measuredGrainDescriptions.addChangeListener(newListener);
	}
	void removeSampleManagementGutsListener(ChangeListener *newListener){
		sampleManager.removeChangeListener(newListener);
	}
	void removeMeasuredGrainDescriptionsListener(ChangeListener *newListener){
		measuredGrainDescriptions.removeChangeListener(newListener);
	}
	int getCurrentWaveSize() const {
		return sampleManager.getLength();
	}
	nvs::service::PresetManager &getPresetManager() { return presetManager; }
	
	nvs::gran::GranularSynthSharedState const &viewSynthSharedState() const;
	void publishAmpBreakpointEnvShape(const nvs::gran::BreakpointEnvShape &shape) {
		jassert (_granularSynth != nullptr);
		_granularSynth->publishAmpBreakpointEnvShape(shape);
	}
protected:
	SlicerGranularAudioProcessor();
	void initialize() {
		initSynth();
		_granularSynth->setLogger([this](const String& message)
		{
			if (FileLogger::getCurrentLogger()){
				loggingGuts.fileLogger.logMessage(message);
			}
		});
		ensureAmpBreakpointEnvInitialized();
	}
	// lazily creates a default breakpoint-envelope shape (mirroring the current ADSR values)
	// if none is persisted yet, then publishes whatever's in the ValueTree to the synth --
	// called on construction and after loading state, so the shared state is always populated
	// even if the editor GUI never opens.
	void ensureAmpBreakpointEnvInitialized();
	virtual void initSynth(){
		// this one-line function gets overriden by TSNGranularAudioProcessor to create a derived type of synthesizer
		_granularSynth = std::make_unique<nvs::gran::GranularSynthesizer>(apvts);
	}


	nvs::util::BroadcastingSampleManager sampleManager;
	nvs::util::LoggingGuts loggingGuts;

	int64 lastLogTimeMs = 0;
	void logRateLimited(const String& message, const int cooldownMs)
	{
        if (const auto now = Time::getMillisecondCounter();
            now - lastLogTimeMs >= cooldownMs)
		{
			lastLogTimeMs = now;
			writeToLog(message);
		}
	}
	
	AudioProcessorValueTreeState apvts;
	nvs::service::PresetManager presetManager;

	std::unique_ptr<nvs::gran::GranularSynthesizer> _granularSynth;
	
	SpinLock audioBlockLock;
	void readIntoBufferAndUpdateState(const File &f);
	
private:
	nvs::util::MeasuredData measuredGrainDescriptions;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlicerGranularAudioProcessor)
};

AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
