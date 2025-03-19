/*
  ==============================================================================

    ScannerParameterPage.h
    Created: 18 Mar 2025 4:17:49pm
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
struct ScannerParameterPage	:	public juce::Component
{
	ScannerParameterPage(juce::AudioProcessorValueTreeState &apvts);
	void resized() override;

private:
	juce::ADSR::Parameters scannerParameters;
	static constexpr int numParams = NUM_SCANNER_PARAMS;
	static_assert(numParams == 2);
	
	std::array<AttachedSlider, numParams> scannerSliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScannerParameterPage)
};
