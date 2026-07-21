/*
  ==============================================================================

    FxParameterPage.h
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./AttachedSlider.h"
#include "./SliderColumn.h"
#include "../Params/params.h"

/*
 like BasicParameterPage, but individual params may opt into a paired
 randomness knob (via SliderColumn) instead of a plain slider.
 */
struct FxParameterPage final : public Component
{
	struct Entry {
		String paramID;
		bool randomized {false};
	};

	FxParameterPage(AudioProcessorValueTreeState &apvts,
					std::initializer_list<Entry> entries,
					Slider::SliderStyle style = Slider::LinearBarVertical);
	void resized() override;

private:
	OwnedArray<Component> columns;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxParameterPage)
};
