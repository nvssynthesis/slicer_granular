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
	static constexpr int numParams =
		static_cast<int>(params_e::count_pos_scan_params )
#ifdef TSN
		- static_cast<int>(params_e::count_nav_lfo_params )
#else
		- static_cast<int>(params_e::count_envelope_params)
#endif
		- 1;
	static_assert(numParams == 2);
	
	std::array<AttachedSlider, numParams> scannerSliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScannerParameterPage)
};
