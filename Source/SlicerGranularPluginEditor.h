/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SlicerGranularPluginProcessor.h"
#include "dsp_util.h"
#include "Gui/FileSelectorComponent.h"
#include "Gui/WaveformComponent.h"
#include "Gui/TabbedPages.h"
#include "Gui/GrainBusyDisplay.h"

//==============================================================================

struct GranularEditorCommon	:	public juce::ChangeListener
,								public juce::FilenameComponentListener
, 								private juce::Timer	// time was only introduced to defer fileComp notating on startup
{
	GranularEditorCommon(Slicer_granularAudioProcessor& p);
	~GranularEditorCommon();	// remove listeners
	// ChangeListener
	void changeListenerCallback (juce::ChangeBroadcaster* source) override;
	// FilenameComponentListener
	void filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged) override;
	// Timer
	void timerCallback() override;
protected:
	void readFile (const juce::File& fileToRead);
	void drawThumbnail(juce::String const &sampleFilePath);
	void notateFileComp(juce::String const &sampleFilePath);
	virtual void displayGrainDescriptions();
	
	void handleSampleManagementBroadcast();
	void handleGrainDescriptionBroadcast();
	//===============================================================================
	WaveformAndPositionComponent waveformAndPositionComponent;
	GrainBusyDisplay grainBusyDisplay;
	FileSelectorComponent fileComp;
	TabbedPagesComponent tabbedPages;

	// to get from processor to draw onto gui
	std::vector<nvs::gran::GrainDescription> grainDescriptions;
	
	Slicer_granularAudioProcessor& audioProcessor;
};

class Slicer_granularAudioProcessorEditor  : 	public juce::AudioProcessorEditor
,												public GranularEditorCommon
{
public:
    Slicer_granularAudioProcessorEditor (Slicer_granularAudioProcessor&);
    ~Slicer_granularAudioProcessorEditor() override;
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
private:
	juce::ComponentBoundsConstrainer constrainer;
	
	std::array<juce::Colour, 5> gradientColors {
		juce::Colours::darkred,
		juce::Colours::darkred,
		juce::Colours::red,
		juce::Colours::darkred,
		juce::Colours::black
	};
	size_t colourOffsetIndex {0};
	
    Slicer_granularAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessorEditor)
};
