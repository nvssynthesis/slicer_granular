//
// Created by Nicholas Solem on 2/11/26.
//

#pragma once
#include <JuceHeader.h>


class ComboBoxHider final  :    public Component
,                               public Timer
{
public:
    explicit ComboBoxHider (StringArray  comboBoxTextAbbreviationsLongToShort) // this is an array of potential text to display on the combo box, to truncate in a way that is sensible to the user
    :   cbTextAbbreviations(std::move(comboBoxTextAbbreviationsLongToShort))
    {
        jassert
        (
            std::ranges::is_sorted(comboBoxTextAbbreviationsLongToShort,
                std::ranges::greater(),
                [](const String &x)
                {
                    return x.length();
                }
            )
        );
        // addChildComponent(cb);
        addAndMakeVisible(cb);
        startTimerHz(30);
    }

    void paint (Graphics &g) override
    {
        for (const auto &txt : cbTextAbbreviations) {
            const auto txtWidth = GlyphArrangement::getStringWidth(g.getCurrentFont(), txt);

            if (const auto txtRect = cb.getBounds()
                .withTrimmedRight(cb.getHeight() + 10).withTrimmedLeft(8).toFloat(); // trim using cb.getHeight() because this should correspond with the little arrow box width in ComboBox
                txtWidth < txtRect.getWidth())
            {
                // then we should use this text and not check any further
                cb.setText(txt);
                break;
            }
        }
    }
    void resized() override
    {
        cb.setBounds(getLocalBounds());
    }
    ComboBox cb;
private:
    void timerCallback() override
    {
        const auto mousePos = Desktop::getMousePosition();
        const bool isInside = getScreenBounds().contains(mousePos);
        cb.setVisible(isInside);
    }
    StringArray cbTextAbbreviations;
};