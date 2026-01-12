/*
  ==============================================================================

    AttachedSlider.cpp
    Created: 22 Jun 2025 5:17:39pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "AttachedSlider.h"

AttachedSlider::AttachedSlider(juce::AudioProcessorValueTreeState &apvts, const ParameterDef& param, const Slider::SliderStyle sliderStyle,
			   const juce::Slider::TextEntryBoxPosition entryPos)
:
_slider(),
_attachment(apvts, param.ID, _slider),
_param_name(param.displayName)
{
	addAndMakeVisible(_slider);
	_slider.setSliderStyle(sliderStyle);
	_slider.setNormalisableRange(param.createNormalisableRange<double>());
	_slider.setTextBoxStyle(entryPos, false, 60, static_cast<int>(_slider.getHeight() * 0.12) );

	_slider.setColour(Slider::ColourIds::thumbColourId, juce::Colours::palevioletred);
	_slider.setColour(Slider::ColourIds::textBoxTextColourId, juce::Colours::lightgrey);
	
	addAndMakeVisible(_label);
	_label.setText(_param_name, juce::dontSendNotification);//nvs::param::ParameterRegistry::getParameterByID(mainParamID).displayName, juce::dontSendNotification
	_label.setFont(FontOptions("Courier New", 13.f, juce::Font::plain));
	_label.setJustificationType(juce::Justification::centred);
}

void AttachedSlider::resized()
{
	if (const auto style = _slider.getSliderStyle();
		style == Slider::SliderStyle::RotaryHorizontalVerticalDrag ||
		style == Slider::SliderStyle::RotaryHorizontalDrag ||
		style == Slider::SliderStyle::RotaryVerticalDrag ||
		style == Slider::SliderStyle::Rotary)
	{
		_slider.setBounds(getLocalBounds());
		return;
	}
	auto r = getLocalBounds();
	// constexpr int extraBottomPadding = 12;
	// r.removeFromBottom (extraBottomPadding);
	const auto bounds = r.toFloat();


	auto const boundsHeight = bounds.getHeight();
	auto const boundsWidth = bounds.getWidth();

    constexpr float sliderProportion = 0.93f;//boundsHeight > 80 ? 0.93f : 0.0f;
	const float labelProportion  = ((boundsWidth > 51) and (boundsHeight > 166)) ? 0.07f : 0.0f;

	juce::FlexBox fb;
	fb.flexDirection  = juce::FlexBox::Direction::column;
	fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	fb.alignItems     = juce::FlexBox::AlignItems::stretch;

	fb.items.add (juce::FlexItem (_slider)
					.withFlex (1.0f*sliderProportion,
							   1.0f,//*sliderProportion,
							   120.0f*sliderProportion));

	if (labelProportion > 0) {
        constexpr float padding = 10.f;
        fb.items.add (juce::FlexItem().withHeight(padding));
	}
	fb.items.add (juce::FlexItem (_label)
					.withFlex (labelProportion,
							   1.0f,//*labelProportion,
							   120.0f*labelProportion));

	fb.performLayout (bounds);
	
	const auto tbW = static_cast<int>(0.66f * static_cast<float>(_slider.getWidth()));
	const auto tbH = static_cast<int>(0.12f * static_cast<float>(_slider.getHeight()));
	const auto textBoxStyle = boundsWidth > 66 && boundsHeight > 146 ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox;

	_slider.setTextBoxStyle (textBoxStyle,
									 false,
									 tbW,
									 tbH);
}
