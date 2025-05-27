/*
  ==============================================================================

    FxPage.cpp
    Created: 26 May 2025 9:52:54pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "FxPage.h"

FxParameterPage::FxParameterPage(juce::AudioProcessorValueTreeState &apvts)
:	fxSliders{
				   AttachedSlider(apvts, params_e::fx_grain_drive, juce::Slider::SliderStyle::LinearBarVertical),
				   AttachedSlider(apvts, params_e::fx_makeup_gain, juce::Slider::SliderStyle::LinearBarVertical)
}
{
	for (auto &s : fxSliders){
		addAndMakeVisible(s._slider);
		s._slider.setTextValueSuffix("linear");
	}
}

void FxParameterPage::resized() {
	auto const bounds = getLocalBounds();
	int const sliderWidth = bounds.getWidth() / numParams;
	int x = 0;
	int const y = bounds.getY();
	int const h = bounds.getHeight();
	for (auto &s : fxSliders){
		s._slider.setBounds(x, y, sliderWidth, h);
		x += sliderWidth;
	}
}
