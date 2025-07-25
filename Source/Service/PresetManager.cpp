/*
  ==============================================================================

    PresetManager.cpp
    Created: 14 Jul 2025 3:38:47pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "PresetManager.h"

namespace nvs::service {

const juce::File PresetManager::defaultDirectory {
	File::getSpecialLocation(File::SpecialLocationType::commonDocumentsDirectory)
							.getChildFile(ProjectInfo::companyName)
							.getChildFile(ProjectInfo::projectName)	// on mac: /Users/Shared/nvssynthesis/csg
};
const juce::String PresetManager::extension{"sgp"};
const juce::String PresetManager::presetNameProperty{"PresetName"};

PresetManager::PresetManager(APVTS& apvts)
:	_apvts(apvts)
{
	// create default preset directory if it doesnt exist
	if (!defaultDirectory.exists()){
		const auto result = defaultDirectory.createDirectory();
		if (result.failed()){
			DBG("Could not create preet directory: " + result.getErrorMessage());
			jassertfalse;
		}
	}
	_apvts.state.addListener(this);
	
	auto fileInfo = _apvts.state.getOrCreateChildWithName("FileInfo", nullptr);
	auto const pname = fileInfo.getPropertyAsValue(presetNameProperty, nullptr);
	currentPreset.referTo( pname );	// writes to state
}
void PresetManager::valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) {
	auto const pname = treeWhichHasBeenChanged.getPropertyAsValue(presetNameProperty, nullptr);
	currentPreset.referTo(pname);	// writes to state
	
	sendChangeMessage();	// inform any listeners (Processor) to update other aspects associated with preset that are not parameters
}

void PresetManager::savePreset(const String &name){
	if (name.isEmpty()){
		return;
	}

	currentPreset.setValue(name);
	
	auto fileInfo = _apvts.state.getOrCreateChildWithName("FileInfo", nullptr);
	fileInfo.setProperty("PresetName", name, nullptr);
	
	const auto xml = _apvts.copyState().createXml();

	const auto presetFile = defaultDirectory.getChildFile(name + "." + extension);
	if (!xml->writeTo(presetFile)){
		DBG("Could not create preset file: " + presetFile.getFullPathName());
		jassertfalse;
	}
}

void PresetManager::deletePreset(const String &name){
	if (name.isEmpty()){
		return;
	}
	
	const auto presetFile = defaultDirectory.getChildFile(name + "." + extension);
	if (!presetFile.existsAsFile()) {
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		jassertfalse;
		return;
	}
	if (!presetFile.deleteFile()){
		DBG("Preset file " + presetFile.getFullPathName() + " could not be deleted");
		jassertfalse;
		return;
	}
	currentPreset.setValue("");
}

void PresetManager::loadPreset(const String &name){
	if (name.isEmpty()){
		return;
	}
	const auto presetFile = defaultDirectory.getChildFile(name + "." + extension);
	
	if (!presetFile.existsAsFile()){
		DBG("Preset file " + presetFile.getFullPathName() + " does not exist");
		jassertfalse;
		return;
	}
	// presetFile (xml) -> valueTree
	juce::XmlDocument xmlDoc { presetFile };
	const auto vtToLoad = juce::ValueTree::fromXml(*xmlDoc.getDocumentElement());
	_apvts.replaceState(vtToLoad);	// in effect calls valueTreeRedirected
	currentPreset.setValue(name);
}

int PresetManager::loadNextPreset(){
	const auto allPresets = getAllPresets();
	if (allPresets.isEmpty()){
		return -1;
	}
	const auto currentIndex = allPresets.indexOf(currentPreset.toString());
	const auto nextIndex = currentIndex >= (allPresets.size() - 1) ? 0 : (currentIndex + 1);
	
	loadPreset(allPresets.getReference(nextIndex));
	return nextIndex;
}

int PresetManager::loadPreviousPreset(){
	const auto allPresets = getAllPresets();
	if (allPresets.isEmpty()){
		return -1;
	}
	const auto currentIndex = allPresets.indexOf(currentPreset.toString());
	const auto prevIndex = currentIndex < 1 ? allPresets.size() - 1 : currentIndex - 1;
	
	loadPreset(allPresets.getReference(prevIndex));
	return prevIndex;
}

juce::StringArray PresetManager::getAllPresets() const {
	StringArray presets;
	const auto fileArray = defaultDirectory.findChildFiles(File::TypesOfFileToFind::findFiles, false,  "*." + extension);
	
	for (const auto & f : fileArray){
		presets.add(f.getFileNameWithoutExtension());
	}
	presets.sortNatural();
	return presets;
}

juce::String PresetManager::getCurrentPreset() const{
	return currentPreset.toString();
}

}
