/*
  ==============================================================================

    EnvelopeParameterPage.h
    Created: 5 May 2024 3:01:02pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
/**
 TODO:
 */
struct EnvelopeParametersPage	:	public juce::Component
{
	EnvelopeParametersPage(juce::AudioProcessorValueTreeState &apvts);
	void resized() override;

private:
	juce::ADSR::Parameters ampEnvParameters;
	juce::Slider attackSlider;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeParametersPage)
};
