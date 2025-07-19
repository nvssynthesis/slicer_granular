/*
  ==============================================================================

    PresetPanel.h
    Created: 14 Jul 2025 4:40:55pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../Service/PresetManager.h"

class PresetPanel		:		public juce::Component
,								private juce::Button::Listener
,								private juce::ComboBox::Listener
{
public:
	PresetPanel(nvs::service::PresetManager &presetManager);
	~PresetPanel();
	void loadPresetList() ;
	
	void comboBoxChanged(juce::ComboBox *cb) override;
	
	void configureButton(juce::Button &b, juce::String const& buttonText);
	void paint(juce::Graphics &g) override;
	void resized() override;
	
private:
	void buttonClicked(juce::Button *b) override;
	nvs::service::PresetManager &_presetManager;
	
	juce::TextButton saveButton, deleteButton, previousPresetButton, nextPresetButton, reloadButton;
	
	juce::ComboBox presetListBox;
	
	std::unique_ptr<juce::FileChooser> fileChooser;
};
