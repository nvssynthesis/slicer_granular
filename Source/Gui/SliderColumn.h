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
	SliderColumn(juce::AudioProcessorValueTreeState &apvts, params_e nonRandomParam)
	:
	_slider(apvts, nonRandomParam, juce::Slider::LinearVertical),
	_knob(apvts, mainToRandom(nonRandomParam), juce::Slider::RotaryHorizontalVerticalDrag)
	{
		addAndMakeVisible(&_slider._slider);
		
		_label.setText(getParamName(nonRandomParam), juce::dontSendNotification);
		
//		_label.setFont( juce::Font("Copperplate", "Regular", proportionOfWidth(0.5f)) );
		_label.setJustificationType(juce::Justification::centred);
		addAndMakeVisible(&_label);
		
		_knob._slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
		addAndMakeVisible(&_knob._slider);
		
		_knobConstrainer.setMinimumSize(knobMinSz, knobMinSz);
	}
	
	void paint(juce::Graphics& g) override {
		_label.setFont( juce::Font("Copperplate", "Regular", proportionOfWidth(0.16f)) );
	}
	void resized() override {
		float sliderProportion {0.75f};
		float labelProportion {0.08f};
		float knobProportion {0.17f};
		[[maybe_unused]] float proportionSum = sliderProportion + labelProportion + knobProportion;
		// would compare with 1 but floating point imprecision
		jassert (proportionSum > 0.999f);
		jassert (proportionSum < 1.001f);
		
		juce::Rectangle<int> localBounds = getLocalBounds();
		localBounds.removeFromBottom(extraBottomPadding);
		
		int const sliderToLabelPadding = 10;
		
		int x(localBounds.getX()), y(localBounds.getY());

		int const sliderHeight = localBounds.getHeight() * sliderProportion - sliderToLabelPadding;
		_slider._slider.setBounds(x, y, localBounds.getWidth(), sliderHeight);
		y += sliderHeight;
		
		y += sliderToLabelPadding;
		
		int const labelHeight = localBounds.getHeight() * labelProportion;// - labelToKnobPadding;
		_label.setBounds(x, y, localBounds.getWidth(), labelHeight);
		y += labelHeight;
		
		
		int const knobHeight = localBounds.getHeight() * knobProportion;
		_knob._slider.setBounds(x, y, localBounds.getWidth(), knobHeight);
		_knobConstrainer.checkComponentBounds(&(_knob._slider));
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
