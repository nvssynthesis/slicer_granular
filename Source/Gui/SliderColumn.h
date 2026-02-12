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
class SliderColumn final :	public Component
{
public:
	SliderColumn(AudioProcessorValueTreeState &apvts, StringRef mainParamID);
	
	void paint(Graphics& g) override;
	void resized() override;
	
	void setVal(double val){
		_slider._slider.setValue(val);
	}
private:
	AttachedSlider _slider;
	AttachedSlider _knob;
	ComponentBoundsConstrainer _knobConstrainer;
	static constexpr int knobMinSz = 60;
};
