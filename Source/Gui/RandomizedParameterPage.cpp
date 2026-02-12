/*
  ==============================================================================

    RandomizedParameterPage.cpp
    Created: 27 May 2025 10:41:07pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "RandomizedParameterPage.h"

RandomizedParameterPage::RandomizedParameterPage(AudioProcessorValueTreeState& apvts,
							std::initializer_list<String> paramIDs) {
    for (const auto& id : paramIDs) {
        const auto s = new SliderColumn (apvts, id);
        sliders.add (s);
    }
    for (const auto &s : sliders) {
        addAndMakeVisible( s );
    }

    {
        auto cb = std::make_unique<ComboBoxHider>(StringArray{"Frequency Options", "Freq. Options", "Freq. Opt.", "Freq Opt", "Options", "Opt.", "Opt", ""});
        cb->cb.addSectionHeading("Randomization Mode");
        cb->cb.addItemList({"Continuous", "Nearest Octave"}, 1);
    #ifdef TSN
        cb->cb.addSectionHeading("Pitchify");
        cb->cb.addItemList({"Yes", "No"}, 3);
    #endif
        comboBoxes.add(std::move(cb));
        comboBoxes[0]->cb.onChange = [this]() {
            const Value val = comboBoxes[0]->cb.getSelectedIdAsValue();
        };
        addAndMakeVisible(comboBoxes[0]);
    }
}
void RandomizedParameterPage::resized() {
	const auto localBounds = getLocalBounds();
    constexpr int freqOptionsMenuBottom = 18;

    const int alottedCompHeight = localBounds.getHeight() - freqOptionsMenuBottom;
	const int alottedCompWidth = localBounds.getWidth() / sliders.size();


	for (int i = 0; i < sliders.size(); ++i){
		const int left = i * alottedCompWidth + localBounds.getX();
		sliders[i]->setBounds(left, freqOptionsMenuBottom, alottedCompWidth, alottedCompHeight);
	}

    if (comboBoxes.size() > 0) {
        const auto b = getBounds();

        constexpr auto h = freqOptionsMenuBottom;
        const auto y = 0;

        const auto cbBounds = Rectangle<int>(1, static_cast<int>(y),
                                            sliders[0]->getWidth(), static_cast<int>(h));
        comboBoxes[0]->setBounds(cbBounds);
    }
}