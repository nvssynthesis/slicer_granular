//
// Created by Nicholas Solem on 9/28/25.
//

#include "AttachedComboBox.h"

AttachedComboBox::AttachedComboBox(APVTS &apvts, const ParameterDef &param)
{
    if (const auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(param.ID))) {
        _comboBox.addItemList(choiceParam->choices, 1);
    }
    addAndMakeVisible(_comboBox);
    _attachment = std::make_unique<Attachment>(apvts, param.ID, _comboBox);
}

void AttachedComboBox::resized() {

    auto r = getLocalBounds();
    constexpr int extraBottomPadding = 2;
    r.removeFromBottom (extraBottomPadding);
    _comboBox.setBounds(r);

    return;

    auto bounds = r.toFloat();


    constexpr float cbProportion = 0.93f;
    constexpr int padding = 10;

    juce::FlexBox fb;
    fb.flexDirection  = juce::FlexBox::Direction::column;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.alignItems     = juce::FlexBox::AlignItems::stretch;

    fb.items.add (juce::FlexItem (_comboBox)
                    .withFlex (1.0f*cbProportion,
                               1.0f,
                               120.0f*cbProportion));

    fb.performLayout (bounds);
 }
