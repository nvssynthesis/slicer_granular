/*
  ==============================================================================

    GranularMainParametersPage.cpp
    Created: 5 May 2024 1:51:43pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularMainParametersPage.h"

GranularMainParametersPage::GranularMainParametersPage(juce::AudioProcessorValueTreeState &apvts)	:	mainParamsComp(apvts)
{
	addAndMakeVisible(mainParamsComp);
}
	
void GranularMainParametersPage::resized() {
	mainParamsComp.setBounds(getLocalBounds());
}

