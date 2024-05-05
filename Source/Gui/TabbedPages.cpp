/*
  ==============================================================================

    TabbedPages.cpp
    Created: 5 May 2024 2:38:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "TabbedPages.h"

TabbedPagesComponent::TabbedPagesComponent (juce::AudioProcessorValueTreeState &apvts)
	: TabbedComponent (juce::TabbedButtonBar::TabsAtTop)
{
	addTab ("Granular Parameters", juce::Colours::transparentWhite, new GranularMainParametersPage(apvts), true);
	addTab ("Envelope Parameters", juce::Colours::transparentWhite, new EnvelopeParametersPage(apvts), true);
}
