/*
  ==============================================================================

    BasicParameterPage.h
    Created: 27 May 2025 10:56:32pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./AttachedSlider.h"
#include "../Params/params.h"

struct BasicParameterPage	:	public juce::Component
{
	BasicParameterPage(juce::AudioProcessorValueTreeState &apvts,
					   std::initializer_list<juce::String> paramIDs,
					   juce::Slider::SliderStyle style = juce::Slider::LinearBarVertical);
	void resized() override;

private:
	juce::OwnedArray<AttachedSlider> sliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicParameterPage)
};
