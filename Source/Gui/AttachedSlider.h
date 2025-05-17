/*
  ==============================================================================

    AttachedSlider.h
    Created: 26 Oct 2023 1:49:14am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../params.h"

struct AttachedSlider {
	using Slider = juce::Slider;
	using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
	using Label = juce::Label;
	
	AttachedSlider(juce::AudioProcessorValueTreeState &apvts, params_e param, Slider::SliderStyle sliderStyle,
				   juce::Slider::TextEntryBoxPosition entryPos = juce::Slider::TextBoxBelow)
	:
	_slider(),
	_attachment(apvts, ::getParamName(param), _slider),
	_param_name(::getParamName(param))
	{
		_slider.setSliderStyle(sliderStyle);
		_slider.setNormalisableRange(getNormalizableRange<double>(param));
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
	
	std::string getParamName() const { return _param_name; }
private:
	std::string _param_name;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AttachedSlider);
};
