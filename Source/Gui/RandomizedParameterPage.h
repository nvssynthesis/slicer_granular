/*
  ==============================================================================

    RandomizedParameterPage.h
    Created: 27 May 2025 10:41:07pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SliderColumn.h"

struct RandomizedParameterPage	:	public juce::Component
{
	RandomizedParameterPage(juce::AudioProcessorValueTreeState& apvts, std::initializer_list<juce::String> paramIDs);
	void resized() override;
private:
	juce::OwnedArray<SliderColumn> sliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizedParameterPage);
};
