/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SlicerGranularPluginProcessor.h"
#include "Gui/PresetPanel.h"
#include "Gui/WaveformComponent.h"
#include "Gui/TabbedPages.h"
#include "Gui/GrainBusyDisplay.h"
#include "../juce_utils/juce_utils.h"

//==============================================================================

struct GranularEditorCommon	:	public ChangeListener
{
    explicit GranularEditorCommon(SlicerGranularAudioProcessor& p);
	~GranularEditorCommon() override;	// remove listeners

	void changeListenerCallback (ChangeBroadcaster* source) override;
protected:
	void drawThumbnail() const;
	virtual void displayGrainDescriptions();
	
	void handleSampleManagementBroadcast();
	void handleGrainDescriptionBroadcast();
	//===============================================================================
	std::unique_ptr<WaveformComponent> waveformComponent;
	GrainBusyDisplay grainBusyDisplay;
	PresetPanel presetPanel;
	TabbedPagesComponent tabbedPages;

	// to get from processor to draw onto gui
	std::vector<nvs::gran::GrainDescription> grainDescriptions;
	
	SlicerGranularAudioProcessor& audioProcessor;
	nvs::util::SampleManagementGuts *sampleManagementGuts {nullptr};
};

inline void displayName(Graphics& g, Rectangle<int> bounds)
{
	g.setColour (Colours::bisque);
	g.setFont (14.0f);
	String const s = String(ProjectInfo::companyName) +
		String(" ") +
		String(ProjectInfo::projectName) +
		String(" version ") +
		String(ProjectInfo::versionString) +
		String("     ");
	g.drawText (s, bounds, Justification::bottomRight, true);
}

class Slicer_granularAudioProcessorEditor final
:   public AudioProcessorEditor
,	public GranularEditorCommon
{
public:
    explicit Slicer_granularAudioProcessorEditor (SlicerGranularAudioProcessor&);
    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
private:	
	std::array<Colour, 5> gradientColors {
		Colours::darkred,
		Colours::darkred,
		Colours::red,
		Colours::darkred,
		Colours::black
	};
	size_t colourOffsetIndex {0};
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessorEditor)
};

inline 
int placeFileCompAndGrainBusyDisplay(const Rectangle<int> localBounds, const int pad, GrainBusyDisplay &grainBusyDisplay, PresetPanel &presetPanel, int yStart) {
    constexpr int fileCompAndGrainDisplayHeight = 26;
	{
		int const grainDisplayHeight = fileCompAndGrainDisplayHeight - pad;
		
		grainBusyDisplay.setSizePerGrain(static_cast<float>(grainDisplayHeight) / static_cast<float>(N_VOICES));
		float const sizePerGrain = grainBusyDisplay.getSizePerGrain();
		
		int const grainBusyDisplayWidth = static_cast<float>(N_GRAINS) * sizePerGrain - static_cast<float>(pad);
		
		int const grainBusyX = localBounds.getX() + (localBounds.getWidth() - grainBusyDisplayWidth) + pad/2;
		int const grainBusyY = yStart + pad/2;
		grainBusyDisplay.setBounds(grainBusyX, grainBusyY, grainBusyDisplayWidth, grainDisplayHeight);
	}
	{
		int const fileCompWidth = localBounds.getWidth() - grainBusyDisplay.getWidth();
		int const x(localBounds.getX());
		presetPanel.setBounds(x, yStart, fileCompWidth, fileCompAndGrainDisplayHeight);
		yStart += fileCompAndGrainDisplayHeight;
		yStart += pad;
	}
	return yStart;	// needs to know the new y to place components at
}
