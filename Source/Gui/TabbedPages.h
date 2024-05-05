/*
  ==============================================================================

    TabbedPages.h
    Created: 5 May 2024 2:38:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./GranularMainParametersPage.h"
#include "./EnvelopeParameterPage.h"

struct TabbedPagesComponent  : public juce::TabbedComponent
{
	TabbedPagesComponent (juce::AudioProcessorValueTreeState &apvts);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabbedPagesComponent)
};
