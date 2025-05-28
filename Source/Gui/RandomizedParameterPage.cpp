/*
  ==============================================================================

    RandomizedParameterPage.cpp
    Created: 27 May 2025 10:41:07pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "RandomizedParameterPage.h"

RandomizedParameterPage::RandomizedParameterPage(juce::AudioProcessorValueTreeState& apvts,
							std::initializer_list<juce::String> paramIDs)
{
	for (auto& id : paramIDs) {
		sliders.add (new SliderColumn (apvts, id));
	}
	for (auto &s : sliders) {
		addAndMakeVisible( s );
	}
}
void RandomizedParameterPage::resized() {
	auto localBounds = getLocalBounds();
	int const alottedCompHeight = localBounds.getHeight();
	int const alottedCompWidth = localBounds.getWidth() / sliders.size();
	
	for (int i = 0; i < sliders.size(); ++i){
		int left = (int)i * alottedCompWidth + localBounds.getX();
		sliders[i]->setBounds(left, 0, alottedCompWidth, alottedCompHeight);
	}
}
