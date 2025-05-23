/*
  ==============================================================================

    MainParamsComponent.h
    Created: 5 Dec 2023 12:39:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../params.h"
#include "./SliderColumn.h"

struct MainParamsComponent	:	public juce::Component
{
	MainParamsComponent(juce::AudioProcessorValueTreeState& apvts)
	:
	attachedSliderColumnArray
	{
		SliderColumn(apvts, params_e::transpose),
		SliderColumn(apvts, params_e::position),
		SliderColumn(apvts, params_e::speed),
		SliderColumn(apvts, params_e::duration),
		SliderColumn(apvts, params_e::skew),
		SliderColumn(apvts, params_e::plateau),
		SliderColumn(apvts, params_e::pan)
	}
	{
		for (auto &s : attachedSliderColumnArray){
			addAndMakeVisible( s );
		}
	}
	void resized() override
	{
		auto localBounds = getLocalBounds();
		int const alottedCompHeight = localBounds.getHeight();// - y + smallPad;
		int const alottedCompWidth = localBounds.getWidth() / attachedSliderColumnArray.size();
		
		for (size_t i = 0; i < attachedSliderColumnArray.size(); ++i){
			int left = (int)i * alottedCompWidth + localBounds.getX();
			attachedSliderColumnArray[i].setBounds(left, 0, alottedCompWidth, alottedCompHeight);
		}
	}
	void setSliderParam(params_e param, double val){
		assert(static_cast<size_t>(param) < (NUM_MAIN_PARAMS / 2) );
		attachedSliderColumnArray[static_cast<size_t>(param)].setVal(val);
	}
private:
	std::array<SliderColumn, NUM_MAIN_PARAMS / 2> attachedSliderColumnArray;
};
