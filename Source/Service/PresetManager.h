/*
  ==============================================================================

    PresetManager.h
    Created: 14 Jul 2025 3:38:47pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

namespace nvs::service {

class PresetManager	:	juce::ValueTree::Listener
{
public:
	using String = juce::String;
	using StringArray = juce::StringArray;
	using APVTS = juce::AudioProcessorValueTreeState;
	using File = juce::File;
	using Value = juce::Value;
	using ValueTree = juce::ValueTree;
	
	static const File defaultDirectory;
	static const String extension;
	static const String presetNameProperty;

	PresetManager(APVTS& apvts);
	void savePreset(const String &name);
	void deletePreset(const String &name);
	void loadPreset(const String &name);
	int loadNextPreset();
	int loadPreviousPreset();
	StringArray getAllPresets() const;
	String getCurrentPreset() const;
private:
	void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override;
	APVTS& _apvts;
	Value currentPreset;
};

}
