/*
  ==============================================================================

    SliderColumn.cpp
    Created: 22 Jun 2025 4:36:04pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "SliderColumn.h"

SliderColumn::SliderColumn(juce::AudioProcessorValueTreeState &apvts, juce::StringRef mainParamID)
:
_slider(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID), juce::Slider::LinearVertical),
_knob(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID + "_rand"), juce::Slider::RotaryHorizontalVerticalDrag)
{
	addAndMakeVisible(&_slider);
//	_knob._slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
	addAndMakeVisible(&_knob);
	_knob._label.setVisible(false);
//	_knobConstrainer.setMinimumSize(knobMinSz, knobMinSz);
}

void SliderColumn::paint(juce::Graphics&) {}
void SliderColumn::resized()
{
	auto r = getLocalBounds();
	constexpr int extraBottomPadding = 12;
	r.removeFromBottom (extraBottomPadding);
	auto bounds = r.toFloat();


	auto const boundsHeight = bounds.getHeight();
	auto const boundsWidth = bounds.getWidth();

	const float sliderProportion = boundsHeight > 80 ? 0.75f : 0.0f;
	const float knobProportion   = 0.17f;
	const int   padding          = 10;

	juce::FlexBox fb;
	fb.flexDirection  = juce::FlexBox::Direction::column;
	fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	fb.alignItems     = juce::FlexBox::AlignItems::stretch;

	fb.items.add (juce::FlexItem (_slider)
					.withFlex (1.0f*sliderProportion,
							   1.0f,//*sliderProportion,
							   120.0f*sliderProportion));

	fb.items.add (juce::FlexItem (_knob)
					.withFlex (1.0f*knobProportion,
							   0.01f*knobProportion,
							   120.0f*knobProportion));

	fb.performLayout (bounds);
}
