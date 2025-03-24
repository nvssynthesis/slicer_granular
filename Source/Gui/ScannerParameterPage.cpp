/*
  ==============================================================================

    ScannerParameterPage.cpp
    Created: 18 Mar 2025 4:17:49pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "ScannerParameterPage.h"

ScannerParameterPage::ScannerParameterPage(juce::AudioProcessorValueTreeState &apvts)
:	scannerSliders{
				   AttachedSlider(apvts, params_e::pos_scan_amount, juce::Slider::SliderStyle::LinearBarVertical),
				   AttachedSlider(apvts, params_e::pos_scan_rate, juce::Slider::SliderStyle::LinearBarVertical)
}
{
	for (auto &s : scannerSliders){
		addAndMakeVisible(s._slider);
		if (s.getParamName() == getParamName(params_e::pos_scan_rate)){
			s._slider.setTextValueSuffix("hz");
		}
	}
}

void ScannerParameterPage::resized() {
	auto const bounds = getLocalBounds();
	int const sliderWidth = bounds.getWidth() / numParams;
	int x = 0;
	int const y = bounds.getY();
	int const h = bounds.getHeight();
	for (auto &s : scannerSliders){
		s._slider.setBounds(x, y, sliderWidth, h);
		x += sliderWidth;
	}
}

