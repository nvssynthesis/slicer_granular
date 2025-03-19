/*
  ==============================================================================

    EnvelopeParameterPage.h
    Created: 5 May 2024 3:01:02pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./AttachedSlider.h"
#include "../params.h"

/**
 TODO:
 */
struct EnvelopeParametersPage	:	public juce::Component
{
	EnvelopeParametersPage(juce::AudioProcessorValueTreeState &apvts);
	void resized() override;

private:
	juce::ADSR::Parameters ampEnvParameters;
	static constexpr int numParams = NUM_ENV_PARAMS;
	static_assert(numParams == 4);
	
	std::array<AttachedSlider, numParams> envelopeSliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeParametersPage)
};
