/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "params.h"

//==============================================================================
/**
*/

class Slicer_granularAudioProcessorEditor  : public juce::AudioProcessorEditor
,			                                 public juce::Slider::Listener
,											 public juce::FilenameComponentListener
//,											 public juce::Timer
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

	std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, static_cast<size_t>(params_e::count)> paramSliderAttachments;

private:
	std::array<juce::Slider, static_cast<size_t>(params_e::count)> paramSliders;
	std::array<juce::Label, static_cast<size_t>(params_e::count)> paramLabels;
    std::unique_ptr<juce::FilenameComponent> fileComp;

	juce::ToggleButton triggeringButton;
	std::array<juce::Colour, 5> gradientColors {
		juce::Colours::transparentBlack,
		juce::Colours::darkred,
		juce::Colours::red,
		juce::Colours::darkred,
		juce::Colours::black
	};
	size_t colourOffsetIndex {0};
	template<typename ExternalStateType>
	struct UpdateState{
		UpdateState(ExternalStateType &s, ExternalStateType externalBound)
		:	ext(s),
			extUpperBound(externalBound)
		{}
		size_t counter {0};
		static constexpr size_t upperBound {20};
		ExternalStateType &ext;
		ExternalStateType extUpperBound;
		bool operator()(){
			++counter;
			if (counter >= upperBound){
				++ext;
				if (ext >= extUpperBound){
					ext = 0;
				}
				counter = 0;
				return true;
			}
			return false;
		}
	};
	UpdateState<size_t> updateState;
	void update()
	{
		const auto needsToRepaint = updateState();
	   
		if (needsToRepaint)
			repaint();
	}
	juce::VBlankAttachment vbAttachment { this, [this] { update(); } };
	
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Slicer_granularAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessorEditor)
};
