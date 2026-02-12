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

struct BasicParameterPage	:	public Component
{
	BasicParameterPage(AudioProcessorValueTreeState &apvts,
					   std::initializer_list<String> paramIDs,
					   Slider::SliderStyle style = Slider::LinearBarVertical);
	void resized() override;

private:
	OwnedArray<AttachedSlider> sliders;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BasicParameterPage)
};
