/*
  ==============================================================================

    FxParameterPage.cpp
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "FxParameterPage.h"

FxParameterPage::FxParameterPage(AudioProcessorValueTreeState &apvts,
								  const std::initializer_list<Entry> entries,
								  Slider::SliderStyle style)
{
	for (const auto &entry : entries) {
		Component *c = entry.randomized
			? static_cast<Component*>(new SliderColumn(apvts, entry.paramID))
			: static_cast<Component*>(new AttachedSlider(apvts, nvs::param::ParameterRegistry::getParameterByID(entry.paramID), style));
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
