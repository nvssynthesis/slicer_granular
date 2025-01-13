/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "dsp_util.h"
#include "Gui/FileSelectorComponent.h"
#include "Gui/WaveformComponent.h"
#include "Gui/TabbedPages.h"

//==============================================================================
/** TODO:
*/

class Slicer_granularAudioProcessorEditor  : 	public juce::AudioProcessorEditor
,												public juce::ChangeListener
,											 	public juce::FilenameComponentListener
, private juce::Timer
{
public:
    Slicer_granularAudioProcessorEditor (Slicer_granularAudioProcessor&);
    ~Slicer_granularAudioProcessorEditor() override;
	//==============================================================================
	void updateToggleState (juce::Button* button, juce::String name, bool &valToAffect);
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
	//===============================================================================
	void changeListenerCallback (juce::ChangeBroadcaster* source) override;
	//===============================================================================
	void filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged) override;
	void timerCallback() override;
private:
	//===============================================================================
	void readFile (const juce::File& fileToRead);
	void drawThumbnail(juce::String const &sampleFilePath);
	void notateFileComp(juce::String const &sampleFilePath);
	//===============================================================================
private:
	juce::ComponentBoundsConstrainer constrainer;

	FileSelectorComponent fileComp;
	TabbedPagesComponent tabbedPages;
	WaveformAndPositionComponent waveformAndPositionComponent;
	
	// to get from processor to draw onto gui
	std::vector<nvs::gran::GrainDescription> grainDescriptions;
	
	
	std::array<juce::Colour, 5> gradientColors {
		juce::Colours::darkred,
		juce::Colours::darkred,
		juce::Colours::red,
		juce::Colours::darkred,
		juce::Colours::black
	};
	size_t colourOffsetIndex {0};
	
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Slicer_granularAudioProcessor& audioProcessor;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessorEditor)
};
