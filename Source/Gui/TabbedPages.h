/*
  ==============================================================================

    TabbedPages.h
    Created: 5 May 2024 2:38:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class SlicerGranularAudioProcessor;

struct TabbedPagesComponent  : public juce::TabbedComponent
{
	TabbedPagesComponent (juce::AudioProcessorValueTreeState &apvts, SlicerGranularAudioProcessor &processor);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabbedPagesComponent)
};
