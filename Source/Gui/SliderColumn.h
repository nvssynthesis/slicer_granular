/*
  ==============================================================================

    SliderColumn.h
    Created: 14 Sep 2023 11:20:22pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "AttachedSlider.h"

/*
 embeds a linear vertical slider, label, and knob into a single component
 */
class SliderColumn	:	public juce::Component
{
public:
	SliderColumn(juce::AudioProcessorValueTreeState &apvts, juce::StringRef mainParamID)
	:
	_slider(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID), juce::Slider::LinearVertical),
	_knob(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID + "_rand"), juce::Slider::RotaryHorizontalVerticalDrag)
	{
		addAndMakeVisible(&_slider._slider);
		_slider._slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, int(float(_slider._slider.getHeight())*0.12));

		_label.setText(nvs::param::ParameterRegistry::getParameterByID(mainParamID).displayName, juce::dontSendNotification);
		
		_label.setJustificationType(juce::Justification::centred);
		addAndMakeVisible(&_label);
		
		_knob._slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
		addAndMakeVisible(&_knob._slider);
		
		_knobConstrainer.setMinimumSize(knobMinSz, knobMinSz);
	}
	
	void paint(juce::Graphics& g) override {
//		_label.setFont( juce::Font("Copperplate", "Regular", proportionOfWidth(0.16f)) );
	}
	void resized() override
	{
		auto r = getLocalBounds();
		r.removeFromBottom (extraBottomPadding);
		auto bounds = r.toFloat();


		auto const boundsHeight = bounds.getHeight();
		auto const boundsWidth = bounds.getWidth();

		const float sliderProportion = boundsHeight > 80 ? 0.75f : 0.0f;
		const float labelProportion  = ((boundsWidth > 51) and (boundsHeight > 166)) ? 0.08f : 0.0f;
		const float knobProportion   = 0.17f;
		const int   padding          = 10;

		juce::FlexBox fb;
		fb.flexDirection  = juce::FlexBox::Direction::column;
		fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
		fb.alignItems     = juce::FlexBox::AlignItems::stretch;

		fb.items.add (juce::FlexItem (_slider._slider)
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

		if (boundsHeight > 146){
			fb.items.add (juce::FlexItem().withHeight ((float) padding));
		}
		fb.items.add (juce::FlexItem (_knob._slider)
						.withFlex (1.0f*knobProportion,
								   0.01f*knobProportion,
								   120.0f*knobProportion));

		fb.performLayout (bounds);
		
		int tbW = int (_slider._slider.getWidth() * 0.66f);
		int tbH = int (_slider._slider.getHeight() * 0.12f);
		auto textBoxStyle = ((boundsWidth > 66) and (boundsHeight > 146)) ? juce::Slider::TextBoxBelow : juce::Slider::NoTextBox;
		_slider._slider.setTextBoxStyle (textBoxStyle,
										 false,
										 tbW,
										 tbH);
	}
	void setVal(double val){
		_slider._slider.setValue(val);
	}
private:
	AttachedSlider _slider;
	juce::Label _label;
	AttachedSlider _knob;
	juce::ComponentBoundsConstrainer _knobConstrainer;
	static constexpr int knobMinSz = 60;
	static constexpr int extraBottomPadding = 12;
};
