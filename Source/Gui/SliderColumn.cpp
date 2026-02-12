/*
  ==============================================================================

    SliderColumn.cpp
    Created: 22 Jun 2025 4:36:04pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "SliderColumn.h"

SliderColumn::SliderColumn(AudioProcessorValueTreeState &apvts, StringRef mainParamID)
:
_slider(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID), Slider::LinearVertical),
_knob(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID + "_rand"), Slider::RotaryHorizontalVerticalDrag)
{
	addAndMakeVisible(&_slider);
//	_knob._slider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
	addAndMakeVisible(&_knob);
	_knob._label.setVisible(false);
//	_knobConstrainer.setMinimumSize(knobMinSz, knobMinSz);
}

void SliderColumn::paint(Graphics&) {}
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

	FlexBox fb;
	fb.flexDirection  = FlexBox::Direction::column;
	fb.justifyContent = FlexBox::JustifyContent::flexStart;
	fb.alignItems     = FlexBox::AlignItems::stretch;

	fb.items.add (FlexItem (_slider)
					.withFlex (1.0f*sliderProportion,
							   1.0f,//*sliderProportion,
							   120.0f*sliderProportion));

	fb.items.add (FlexItem (_knob)
					.withFlex (1.0f*knobProportion,
							   0.01f*knobProportion,
							   120.0f*knobProportion));

	fb.performLayout (bounds);
}
