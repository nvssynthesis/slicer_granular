/*
  ==============================================================================

    TabbedPages.cpp
    Created: 5 May 2024 2:38:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "TabbedPages.h"
#include "./GranularMainParametersPage.h"
#include "./EnvelopeParameterPage.h"
#include "./ScannerParameterPage.h"
#include "./FxPage.h"

TabbedPagesComponent::TabbedPagesComponent (juce::AudioProcessorValueTreeState &apvts)
	: TabbedComponent (juce::TabbedButtonBar::TabsAtTop)
{
	auto &bar = getTabbedButtonBar();
	bar.setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::grey);			// colour for unselected tab text
	bar.setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::snow);		// colour for selected tab text

	addTab ("Granular", juce::Colours::transparentWhite, new GranularMainParametersPage(apvts), true);
	addTab ("Envelope", juce::Colours::transparentWhite, new EnvelopeParametersPage(apvts), true);
	addTab ("Scanner", juce::Colours::transparentWhite, new ScannerParameterPage(apvts), true);
	addTab ("Fx", juce::Colours::transparentWhite, new FxParameterPage(apvts), true);
}
