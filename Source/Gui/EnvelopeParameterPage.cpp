/*
  ==============================================================================

    EnvelopeParameterPage.cpp
    Created: 5 May 2024 3:01:02pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "EnvelopeParameterPage.h"
EnvelopeParametersPage::EnvelopeParametersPage(juce::AudioProcessorValueTreeState &apvts)
:	envelopeSliders{
	AttachedSlider(apvts, params_e::amp_attack, juce::Slider::SliderStyle::LinearBarVertical),
	AttachedSlider(apvts, params_e::amp_decay, juce::Slider::SliderStyle::LinearBarVertical),
	AttachedSlider(apvts, params_e::amp_sustain, juce::Slider::SliderStyle::LinearBarVertical),
	AttachedSlider(apvts, params_e::amp_release, juce::Slider::SliderStyle::LinearBarVertical)
}
{
	for (auto &s : envelopeSliders){

		addAndMakeVisible(s._slider);
		if (s.getParamName() != getParamName(params_e::amp_sustain)) {
			s._slider.setTextValueSuffix("s");
		}
	}
}

void EnvelopeParametersPage::resized() {
	auto const bounds = getLocalBounds();
	int const sliderWidth = bounds.getWidth() / numParams;
	int x = 0;
	int const y = bounds.getY();
	int const h = bounds.getHeight();
	for (auto &s : envelopeSliders){
		s._slider.setBounds(x, y, sliderWidth, h);
		x += sliderWidth;
	}
}
