/*
  ==============================================================================

    AttachedSlider.h
    Created: 26 Oct 2023 1:49:14am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../Params/params.h"

class AttachedSlider	:	public juce::Component
{
	using ParameterDef = nvs::param::ParameterDef;
	using Slider = juce::Slider;
	using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	using Label = juce::Label;
	using String = juce::String;

public:
	AttachedSlider(juce::AudioProcessorValueTreeState &apvts, ParameterDef param, Slider::SliderStyle sliderStyle,
				   juce::Slider::TextEntryBoxPosition entryPos = juce::Slider::TextBoxBelow);
	
	void resized() override;
	
	Slider _slider;
	SliderAttachment _attachment;
	Label _label;

	String getParamName() const { return _param_name; }
private:
	String _param_name;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AttachedSlider);
};
