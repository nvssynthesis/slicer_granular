/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "params.h"
#include "dsp_util.h"
#include "FileSelectorComponent.h"
#include "SliderColumn.h"
#include "WaveformComponent.h"

//==============================================================================
/** TODO:
*/

struct MainParamsComponent	:	public juce::Component
{
	MainParamsComponent(Slicer_granularAudioProcessor& p)
	:
	attachedSliderColumnArray {
		SliderColumn(p.apvts, params_e::transpose),
		SliderColumn(p.apvts, params_e::position),
		SliderColumn(p.apvts, params_e::speed),
		SliderColumn(p.apvts, params_e::duration),
		SliderColumn(p.apvts, params_e::skew),
		SliderColumn(p.apvts, params_e::plateau),
		SliderColumn(p.apvts, params_e::pan)
	}
	{
		for (auto &s : attachedSliderColumnArray){
			addAndMakeVisible( s );
		}
	}
	void resized() override
	{
		auto localBounds = getLocalBounds();
		int const alottedCompHeight = localBounds.getHeight();// - y + smallPad;
		int const alottedCompWidth = localBounds.getWidth() / attachedSliderColumnArray.size();
		
		for (int i = 0; i < attachedSliderColumnArray.size(); ++i){
			int left = i * alottedCompWidth + localBounds.getX();
			attachedSliderColumnArray[i].setBounds(left, 0, alottedCompWidth, alottedCompHeight);
		}
	}
	
private:
	std::array<SliderColumn, static_cast<size_t>(params_e::count) / 2> attachedSliderColumnArray;
};

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
	MainParamsComponent mainParamsComp;
	WaveformAndPositionComponent waveformAndPositionComponent;

/*
    std::unique_ptr<juce::FilenameComponent> fileComp;
	juce::StringArray recentFiles;
	juce::File recentFilesListFile;
*/
	juce::ToggleButton triggeringButton;
	std::array<juce::Colour, 5> gradientColors {
		juce::Colours::transparentBlack,
		juce::Colours::darkred,
		juce::Colours::red,
		juce::Colours::darkred,
		juce::Colours::black
	};
	size_t colourOffsetIndex {0};

	void update()
	{
		return;
//		const auto needsToRepaint = updateState();
	   
		const float level = audioProcessor.rmsInformant.val;
		const float recentLevel = audioProcessor.rmsWAinformant.val;

		const bool needsToRepaint = (level > (recentLevel * 1.2f));
		
		if (needsToRepaint){
			++colourOffsetIndex;
			colourOffsetIndex %= gradientColors.size();
			repaint();
		}
	}
	juce::VBlankAttachment vbAttachment { this, [this] { update(); } };
	
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Slicer_granularAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessorEditor)
};
