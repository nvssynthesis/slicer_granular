/*
  ==============================================================================

    TabbedPages.cpp
    Created: 5 May 2024 2:38:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "TabbedPages.h"
#include "./RandomizedParameterPage.h"
#include "./BasicParameterPage.h"

TabbedPagesComponent::TabbedPagesComponent (juce::AudioProcessorValueTreeState &apvts)
	: TabbedComponent (juce::TabbedButtonBar::TabsAtTop)
{
	auto &bar = getTabbedButtonBar();
	bar.setColour(juce::TabbedButtonBar::tabTextColourId, juce::Colours::grey);			// colour for unselected tab text
	bar.setColour(juce::TabbedButtonBar::frontTextColourId, juce::Colours::snow);		// colour for selected tab text

	addTab ("Granular", juce::Colours::transparentWhite,
			new RandomizedParameterPage(apvts,
										{	"transpose",
											"position",
											"speed",
											"duration",
											"skew",
											"plateau",
											"pan"
										}), true);
	addTab ("Envelope", juce::Colours::transparentWhite,
			new BasicParameterPage(apvts,
								   {"amp_env_attack",
									"amp_env_decay",
									"amp_env_sustain",
									"amp_env_release"
									}//, juce::Slider::SliderStyle::LinearVertical
								   ), true);
	addTab ("Scanner", juce::Colours::transparentWhite, new BasicParameterPage(apvts,
																			   {"scanner_rate", "scanner_amount"}, juce::Slider::SliderStyle::LinearVertical
																			   ), true);
	addTab ("Fx", juce::Colours::transparentWhite, new BasicParameterPage(apvts,
																		  {"fx_grain_drive", "fx_makeup_gain"}, juce::Slider::SliderStyle::LinearVertical
																		  ), true);
}
