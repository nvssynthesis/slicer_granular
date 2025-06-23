/*
  ==============================================================================

    BasicParameterPage.cpp
    Created: 27 May 2025 10:56:32pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "BasicParameterPage.h"

BasicParameterPage::BasicParameterPage(juce::AudioProcessorValueTreeState &apvts,
									   std::initializer_list<juce::String> paramIDs,
									   juce::Slider::SliderStyle style)
{
	for (auto& id : paramIDs) {
		sliders.add (new AttachedSlider (apvts, nvs::param::ParameterRegistry::getParameterByID(id), style));
	}
	for (auto &s : sliders) {
		addAndMakeVisible(s);
	}
}

void BasicParameterPage::resized() {
	auto const bounds = getLocalBounds();
	int const sliderWidth = bounds.getWidth() / sliders.size();
	int x = 0;
	int const y = bounds.getY();
	int const h = bounds.getHeight();
	for (auto &s : sliders){
		s->setBounds(x, y, sliderWidth, h);
		x += sliderWidth;
	}
}
