/*
  ==============================================================================

    FxParameterPage.h
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./AttachedSlider.h"
#include "./AttachedComboBox.h"
#include "./SliderColumn.h"
#include "../Params/params.h"

/*
 like BasicParameterPage, but individual params may opt into a paired
 randomness knob (via SliderColumn), or a discrete choice (via ComboBox)
 paired with a randomness knob, instead of a plain slider.
 */
struct FxParameterPage final : public Component
{
	struct Entry {
		enum class Kind { Plain, Randomized, Choice };
		String paramID;
		Kind kind {Kind::Plain};
	};

	FxParameterPage(AudioProcessorValueTreeState &apvts,
					std::initializer_list<Entry> entries,
					Slider::SliderStyle style = Slider::LinearBarVertical);
	void resized() override;

private:
	// a discrete choice (ComboBox) paired with a randomness knob bound to paramID + "_rand"
	struct ComboBoxColumn final : public Component
	{
		ComboBoxColumn(AudioProcessorValueTreeState &apvts, StringRef mainParamID);
		void resized() override;
	private:
		AttachedComboBox _comboBox;
		AttachedSlider _knob;
	};

	OwnedArray<Component> columns;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxParameterPage)
};
