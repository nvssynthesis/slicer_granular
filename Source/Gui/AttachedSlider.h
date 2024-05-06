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
	
	AttachedSlider(juce::AudioProcessorValueTreeState &apvts, params_e param, Slider::SliderStyle sliderStyle,
				   juce::Slider::TextEntryBoxPosition entryPos = juce::Slider::TextBoxBelow)
	:
	_slider(),
	_attachment(apvts, getParamName(param), _slider)
	{
		_slider.setSliderStyle(sliderStyle);
		_slider.setNormalisableRange(getNormalizableRange<double>(param));
		_slider.setTextBoxStyle(entryPos, false, 50, 25);
		_slider.setValue(getParamDefault(param));

		_slider.setColour(Slider::ColourIds::thumbColourId, juce::Colours::palevioletred);
		_slider.setColour(Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);
	}
	
	Slider _slider;
	SliderAttachment _attachment;
};
