/*
  ==============================================================================

    AttachedSlider.cpp
    Created: 22 Jun 2025 5:17:39pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "AttachedSlider.h"

AttachedSlider::AttachedSlider(juce::AudioProcessorValueTreeState &apvts, ParameterDef param, Slider::SliderStyle sliderStyle,
			   juce::Slider::TextEntryBoxPosition entryPos)
:
_slider(),
_attachment(apvts, param.ID, _slider),
_param_name(param.displayName)
{
	addAndMakeVisible(_slider);
	_slider.setSliderStyle(sliderStyle);
	_slider.setNormalisableRange(param.createNormalisableRange<double>());
//	_slider.setTextBoxStyle(entryPos, false, 50, 25);
	_slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, int(_slider.getHeight()*0.12) );

	_slider.setColour(Slider::ColourIds::thumbColourId, juce::Colours::palevioletred);
	_slider.setColour(Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);
	
	addAndMakeVisible(_label);
	_label.setText(_param_name, juce::dontSendNotification);//nvs::param::ParameterRegistry::getParameterByID(mainParamID).displayName, juce::dontSendNotification
	_label.setFont(juce::Font("Courier New", 13.f, juce::Font::plain));
	_label.setJustificationType(juce::Justification::centred);
}

void AttachedSlider::resized()
{
	if (auto style = _slider.getSliderStyle();
		style == Slider::SliderStyle::RotaryHorizontalVerticalDrag ||
		style == Slider::SliderStyle::RotaryHorizontalDrag ||
		style == Slider::SliderStyle::RotaryVerticalDrag ||
		style == Slider::SliderStyle::Rotary)
	{
		_slider.setBounds(getLocalBounds());
		return;
	}
	auto r = getLocalBounds();
	constexpr int extraBottomPadding = 12;
	r.removeFromBottom (extraBottomPadding);
	auto bounds = r.toFloat();


	auto const boundsHeight = bounds.getHeight();
	auto const boundsWidth = bounds.getWidth();

	const float sliderProportion = 0.93f;//boundsHeight > 80 ? 0.93f : 0.0f;
	const float labelProportion  = ((boundsWidth > 51) and (boundsHeight > 166)) ? 0.07f : 0.0f;
	const int   padding          = 10;

	juce::FlexBox fb;
	fb.flexDirection  = juce::FlexBox::Direction::column;
	fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	fb.alignItems     = juce::FlexBox::AlignItems::stretch;

	fb.items.add (juce::FlexItem (_slider)
					.withFlex (1.0f*sliderProportion,
							   1.0f,//*sliderProportion,
							   120.0f*sliderProportion));

	if (labelProportion > 0){
		fb.items.add (juce::FlexItem().withHeight ((float) padding));
	}
	fb.items.add (juce::FlexItem (_label)
					.withFlex (labelProportion,
							   1.0f,//*labelProportion,
							   120.0f*labelProportion));

	fb.performLayout (bounds);
	
	int tbW = int (_slider.getWidth() * 0.66f);
	int tbH = int (_slider.getHeight() * 0.12f);
	auto textBoxStyle = ((boundsWidth > 66) and (boundsHeight > 146)) ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox;
	_slider.setTextBoxStyle (textBoxStyle,
									 false,
									 tbW,
									 tbH);
}
