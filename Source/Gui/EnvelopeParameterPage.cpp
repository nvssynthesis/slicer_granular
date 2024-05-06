/*
  ==============================================================================

    EnvelopeParameterPage.cpp
    Created: 5 May 2024 3:01:02pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "EnvelopeParameterPage.h"

EnvelopeParametersPage::EnvelopeParametersPage(juce::AudioProcessorValueTreeState &apvts){
	attackSlider.setSliderStyle(juce::Slider::SliderStyle::LinearBarVertical);
	attackSlider.setTitle("atk");
	attackSlider.setName("attlk");
	attackSlider.setHelpText("sets attack");
	attackSlider.setDescription("it sets attack");
	attackSlider.setTooltip("this sets atk");
	attackSlider.setTextValueSuffix("s");
	attackSlider.setNumDecimalPlacesToDisplay(2);
	addAndMakeVisible(attackSlider);

}

void EnvelopeParametersPage::resized() {
	int constexpr nParams = 4;
	auto const bounds = getLocalBounds();
	int const sliderWidth = bounds.getWidth() / nParams;
	auto x = 0;
	attackSlider.setBounds(x, 0, sliderWidth, bounds.getHeight());
}
