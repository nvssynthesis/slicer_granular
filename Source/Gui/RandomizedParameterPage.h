/*
  ==============================================================================

    RandomizedParameterPage.h
    Created: 27 May 2025 10:41:07pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SliderColumn.h"
#include "ComboBoxHider.h"

struct RandomizedParameterPage final :	public Component
{
	RandomizedParameterPage(AudioProcessorValueTreeState& apvts, std::initializer_list<String> paramIDs);
	void resized() override;
private:
	OwnedArray<SliderColumn> sliders;

    OwnedArray<ComboBoxHider> comboBoxes;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RandomizedParameterPage)
};
