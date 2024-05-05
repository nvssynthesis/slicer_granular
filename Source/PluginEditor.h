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

class Slicer_granularAudioProcessorEditor  : public juce::AudioProcessorEditor
,			                                 public juce::Slider::Listener
,											 public juce::FilenameComponentListener
{
public:
    Slicer_granularAudioProcessorEditor (Slicer_granularAudioProcessor&);
    ~Slicer_granularAudioProcessorEditor() override;
	//==============================================================================
	void updateToggleState (juce::Button* button, juce::String name, bool &valToAffect);
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
	
	void sliderValueChanged(juce::Slider*) override;
	
	//===============================================================================
	void filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged) override;
	void readFile (const juce::File& fileToRead);
	
	//===============================================================================

private:
	juce::ComponentBoundsConstrainer constrainer;

	FileSelectorComponent fileComp;
	TabbedPagesComponent tabbedPages;
	WaveformAndPositionComponent waveformAndPositionComponent;

	juce::ToggleButton triggeringButton;
	std::array<juce::Colour, 5> gradientColors {
		juce::Colours::transparentBlack,
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
