/*
  ==============================================================================

    GranularMainParametersPage.h
    Created: 5 May 2024 1:51:43pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./MainParamsComponent.h"
/**
 TODO:
 */
struct GranularMainParametersPage	:	public juce::Component
{
	GranularMainParametersPage(juce::AudioProcessorValueTreeState &apvts);
	void resized() override;
private:
	MainParamsComponent mainParamsComp;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularMainParametersPage)
};
