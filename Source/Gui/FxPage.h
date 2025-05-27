/*
  ==============================================================================

    FxPage.h
    Created: 26 May 2025 9:52:54pm
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
struct FxParameterPage	:	public juce::Component
{
	FxParameterPage(juce::AudioProcessorValueTreeState &apvts);
	void resized() override;

private:
	static constexpr int numParams = NUM_FX_PARAMS;
	static_assert(numParams == 2);
	
	std::array<AttachedSlider, numParams> fxSliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxParameterPage)
};
