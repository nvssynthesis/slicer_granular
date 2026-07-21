/*
  ==============================================================================

    FxParameterPage.cpp
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "FxParameterPage.h"

FxParameterPage::ComboBoxColumn::ComboBoxColumn(AudioProcessorValueTreeState &apvts, StringRef mainParamID)
:   _comboBox(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID))
,   _knob(apvts, nvs::param::ParameterRegistry::getParameterByID(mainParamID + "_rand"), Slider::RotaryHorizontalVerticalDrag)
{
	addAndMakeVisible(_comboBox);
	addAndMakeVisible(_knob);
	_knob._label.setVisible(false);
}

void FxParameterPage::ComboBoxColumn::resized() {
	auto r = getLocalBounds();
	constexpr int comboHeight = 24;
	_comboBox.setBounds(r.removeFromTop(comboHeight).reduced(2, 0));
	r.removeFromTop(4);
	_knob.setBounds(r);
}

FxParameterPage::FxParameterPage(AudioProcessorValueTreeState &apvts,
								  const std::initializer_list<Entry> entries,
								  Slider::SliderStyle style)
{
	for (const auto &entry : entries) {
		Component *c = nullptr;
		switch (entry.kind) {
			case Entry::Kind::Randomized:	c = new SliderColumn(apvts, entry.paramID); break;
			case Entry::Kind::Choice:		c = new ComboBoxColumn(apvts, entry.paramID); break;
			case Entry::Kind::Plain:
			default:						c = new AttachedSlider(apvts, nvs::param::ParameterRegistry::getParameterByID(entry.paramID), style); break;
		}
		columns.add(c);
	}
	for (auto *c : columns) {
		addAndMakeVisible(c);
	}
}

void FxParameterPage::resized() {
	auto const bounds = getLocalBounds();
	int const columnWidth = bounds.getWidth() / columns.size();
	int x = bounds.getX();
	int const y = bounds.getY();
	int const h = bounds.getHeight();
	for (auto *c : columns) {
		c->setBounds(x, y, columnWidth, h);
		x += columnWidth;
	}
}
