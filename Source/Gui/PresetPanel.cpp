/*
  ==============================================================================

    PresetPanel.cpp
    Created: 14 Jul 2025 4:40:55pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "PresetPanel.h"


PresetPanel::PresetPanel(nvs::service::PresetManager &presetManager)
:	_presetManager(presetManager)
{
	configureButton(saveButton, "Save");
	configureButton(deleteButton, "Delete");
	configureButton(previousPresetButton, "<");
	configureButton(nextPresetButton, ">");
	configureButton(reloadButton, "Reload");
	
	presetListBox.setTextWhenNothingSelected("No Preset Selected");
	presetListBox.setMouseCursor(juce::MouseCursor::PointingHandCursor);
	addAndMakeVisible(presetListBox);
	presetListBox.addListener(this);
	
	loadPresetList();
}
PresetPanel::~PresetPanel() {
	saveButton.removeListener(this);
	deleteButton.removeListener(this);
	previousPresetButton.removeListener(this);
	nextPresetButton.removeListener(this);
	reloadButton.removeListener(this);
	presetListBox.removeListener(this);
}
void PresetPanel::loadPresetList() {
	presetListBox.clear(juce::dontSendNotification);
	
	const auto allPresets = _presetManager.getAllPresets();
	const auto currentPreset = _presetManager.getCurrentPreset();
	presetListBox.addItemList(allPresets, 1);
	presetListBox.setSelectedItemIndex(allPresets.indexOf(currentPreset), juce::dontSendNotification);
}

void PresetPanel::comboBoxChanged(juce::ComboBox *cb) {
	if (cb == &presetListBox){
		_presetManager.loadPreset(presetListBox.getItemText(presetListBox.getSelectedItemIndex()));
	}
}

void PresetPanel::configureButton(juce::Button &b, juce::String const& buttonText){
	b.setButtonText(buttonText);
	b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
	addAndMakeVisible(b);
	b.addListener(this);
}
void PresetPanel::paint(juce::Graphics &g) {
	using namespace juce;
	auto const p0 = Point<float>(getX(), getY());
	auto const p1 = p0 + Point<float>(getWidth(), getHeight());
	g.setGradientFill(ColourGradient(Colour(Colours::lightgrey).withMultipliedAlpha(0.1f), p0,
									 Colour(Colours::grey).withMultipliedAlpha(0.3f), p1, false));
	g.fillRect(getLocalBounds());
}
void PresetPanel::resized() {
	using namespace juce;

	auto const area = getLocalBounds().toFloat();
	// set up flex container
	FlexBox fb;
	fb.flexDirection = FlexBox::Direction::row;
	fb.alignItems    = FlexBox::AlignItems::stretch;
	fb.alignContent  = FlexBox::AlignContent::spaceAround;
	
	float const margin = 4.0f;
	// add your controls with the same relative widths and 1px margin
	float const txtMinW = 36.f;
	float charMinW = 18.f;
	float const textMaxW = area.getWidth() < 286 ? 0.f : 1000.f;
	float const charMaxW = area.getWidth() < 172 ? 0.f : 1000.f;
	fb.items = {
		FlexItem (saveButton)               .withFlex (0.08f).withMargin (margin).withMinWidth(txtMinW).withMaxWidth(textMaxW),
		FlexItem (reloadButton)				.withFlex (0.08f).withMargin (margin).withMinWidth(txtMinW).withMaxWidth(textMaxW),
		FlexItem (previousPresetButton)     .withFlex (0.03f).withMargin (margin).withMinWidth(charMinW).withMaxWidth(charMaxW),
		FlexItem (presetListBox)            .withFlex (0.60f).withMargin (margin).withMinWidth(96.f),
		FlexItem (nextPresetButton)         .withFlex (0.03f).withMargin (margin).withMinWidth(charMinW).withMaxWidth(charMaxW),
		FlexItem (deleteButton)             .withFlex (0.08f).withMargin (margin).withMinWidth(txtMinW).withMaxWidth(textMaxW)
	};
	
	// perform the layout in our component's bounds
	fb.performLayout (area);
}

void PresetPanel::buttonClicked(juce::Button *b) {
	using namespace juce;
	
	if (b == &saveButton) {
		fileChooser = std::make_unique<juce::FileChooser>(
			"Enter preset name to save...",
			nvs::service::PresetManager::defaultDirectory,
			"*." + nvs::service::PresetManager::extension
		);
		fileChooser->launchAsync(FileBrowserComponent::saveMode, [&](const juce::FileChooser &fc)
		{
			const auto resultFile = fc.getResult();
			_presetManager.savePreset(resultFile.getFileNameWithoutExtension());
			loadPresetList();
		});
	}
	if (b == &previousPresetButton){
		const auto idx = _presetManager.loadPreviousPreset();
		presetListBox.setSelectedItemIndex(idx, dontSendNotification);
	}
	if (b == &nextPresetButton){
		const auto idx = _presetManager.loadNextPreset();
		presetListBox.setSelectedItemIndex(idx, dontSendNotification);
	}
	if (b == &deleteButton){
		[[maybe_unused]]
		bool const button1Clicked = juce::AlertWindow::showOkCancelBox(
			juce::AlertWindow::WarningIcon,
			"Delete Preset",
			"Are you sure you want to delete the preset '" +
			_presetManager.getCurrentPreset() + "'?\n\nThis action cannot be undone.",
			"Delete",
			"Cancel",
			this,
			juce::ModalCallbackFunction::create([this](int result) {
				if (result == 1) { // User clicked "Delete"
					_presetManager.deletePreset(_presetManager.getCurrentPreset());
					loadPresetList();
				}
				// If result == 0, user clicked "Cancel" - do nothing
			})
		);
	}
	if (b == &reloadButton){
		std::cerr << "no implementation yet\n";
	}
}
