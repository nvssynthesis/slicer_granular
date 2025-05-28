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

struct AttachedSlider {
	using ParameterDef = nvs::param::ParameterDef;
	using Slider = juce::Slider;
	using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	using Label = juce::Label;
	using String = juce::String;
	
	AttachedSlider(juce::AudioProcessorValueTreeState &apvts, ParameterDef param, Slider::SliderStyle sliderStyle,
				   juce::Slider::TextEntryBoxPosition entryPos = juce::Slider::TextBoxBelow)
	:
	_slider(),
	_attachment(apvts, param.ID, _slider),
	_param_name(param.displayName)
	{
		_slider.setSliderStyle(sliderStyle);
		_slider.setNormalisableRange(param.createNormalisableRange<double>());
		_slider.setTextBoxStyle(entryPos, false, 50, 25);

		_slider.setColour(Slider::ColourIds::thumbColourId, juce::Colours::palevioletred);
		_slider.setColour(Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);
		
		_label.setText(_param_name, juce::dontSendNotification);
		_label.setFont(juce::Font("Courier New", 13.f, juce::Font::plain));
		_label.setJustificationType(juce::Justification::centred);
	}
	
	
	Slider _slider;
	SliderAttachment _attachment;
	Label _label;
	
	String getParamName() const { return _param_name; }
private:
	String _param_name;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AttachedSlider);
};
