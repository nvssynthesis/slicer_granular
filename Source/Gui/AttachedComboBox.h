//
// Created by Nicholas Solem on 9/28/25.
//

#ifndef ATTACHEDCOMBOBOX_H
#define ATTACHEDCOMBOBOX_H
#include <JuceHeader.h>
#include "../Params/params.h"

class AttachedComboBox  :   public juce::Component {
    using ParameterDef = nvs::param::ParameterDef;
    using ComboBox = juce::ComboBox;
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::ComboBoxAttachment;
    using Label = juce::Label;
    using String = juce::String;
    using StringArray = juce::StringArray;
public:
    AttachedComboBox(APVTS &apvts, const ParameterDef &param);
    void resized() override;

    String getParamName() const {
        return _param_name;
    }

    ComboBox _comboBox;
private:
    std::unique_ptr<Attachment> _attachment;
    String _param_name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AttachedComboBox);
};


#endif //ATTACHEDCOMBOBOX_H
