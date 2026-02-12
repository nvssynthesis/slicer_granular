/*
  ==============================================================================

    RandomizedParameterPage.cpp
    Created: 27 May 2025 10:41:07pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "RandomizedParameterPage.h"

RandomizedParameterPage::RandomizedParameterPage(AudioProcessorValueTreeState& apvts,
							const std::initializer_list<String> paramIDs) {
    for (const auto& id : paramIDs) {
        const auto s = new SliderColumn (apvts, id);
        sliders.add (s);
    }
    for (const auto &s : sliders) {
        addAndMakeVisible( s );
    }

    {
        auto cb = std::make_unique<ComboBoxHider>(StringArray{"Frequency Options", "Freq. Options", "Freq. Opt.", "Freq Opt", "Options", "Opt.", "Opt", ""});
        ComboBox &underlyingCB = cb->cb;
        underlyingCB.addSectionHeading("Randomization Mode");
        underlyingCB.addItemList({nvs::axiom::Continuous, nvs::axiom::Octaves}, 1);
    #ifdef TSN
        underlyingCB.addSectionHeading("Pitchify");
        underlyingCB.addItemList({"On", "Off"}, 3);
    #endif
        comboBoxes.add(std::move(cb));
        comboBoxes[0]->cb.onChange = [this, &apvts]() {
            const Value val = comboBoxes[0]->cb.getSelectedIdAsValue();
            switch (static_cast<int>(val.getValue())) {
                [[unlikely]]
                default: {
                    break;
                }
                case 0:
                    break;
                case 1: {
                    RangedAudioParameter *fRandModeParam = apvts.getParameter(nvs::axiom::frequency_randomization_mode);
                    if (fRandModeParam != nullptr) {
                        fRandModeParam->setValueNotifyingHost(0.f);
                    }
                    break;
                }
                case 2: {
                    RangedAudioParameter *fRandModeParam = apvts.getParameter(nvs::axiom::frequency_randomization_mode);
                    if (fRandModeParam != nullptr) {
                        fRandModeParam->setValueNotifyingHost(1.f);
                    }
                    break;
                }
#ifdef TSN
                case 3: {
                    RangedAudioParameter *pitchifyParam = apvts.getParameter(nvs::axiom::tsn::pitchify);
                    if (pitchifyParam != nullptr) {
                        pitchifyParam->setValueNotifyingHost(0.f);
                    }
                    break;
                }
                case 4: {
                    RangedAudioParameter *pitchifyParam = apvts.getParameter(nvs::axiom::tsn::pitchify);
                    if (pitchifyParam != nullptr) {
                        pitchifyParam->setValueNotifyingHost(1.f);
                    }
                    break;
                }
#endif
            }
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