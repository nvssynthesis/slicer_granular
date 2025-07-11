/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SlicerGranularPluginProcessor.h"
#include "dsp_util.h"
#include "Gui/FileSelectorComponent.h"
#include "Gui/WaveformComponent.h"
#include "Gui/TabbedPages.h"
#include "Gui/GrainBusyDisplay.h"

//==============================================================================

struct GranularEditorCommon	:	public juce::ChangeListener
,								public juce::FilenameComponentListener
, 								private juce::Timer	// time was only introduced to defer fileComp notating on startup
{
	GranularEditorCommon(SlicerGranularAudioProcessor& p);
	~GranularEditorCommon();	// remove listeners
	// ChangeListener
	void changeListenerCallback (juce::ChangeBroadcaster* source) override;
	// FilenameComponentListener
	void filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged) override;
	// Timer
	void timerCallback() override;
protected:
	void drawThumbnail();
	void notateFileComp(juce::String const &sampleFilePath);
	virtual void displayGrainDescriptions();
	
	void handleSampleManagementBroadcast();
	void handleGrainDescriptionBroadcast();
	//===============================================================================
	WaveformAndPositionComponent waveformAndPositionComponent;
	GrainBusyDisplay grainBusyDisplay;
	FileSelectorComponent fileComp;
	TabbedPagesComponent tabbedPages;

	// to get from processor to draw onto gui
	std::vector<nvs::gran::GrainDescription> grainDescriptions;
	
	SlicerGranularAudioProcessor& audioProcessor;
	nvs::util::SampleManagementGuts *sampleManagementGuts {nullptr};
};

inline void displayName(juce::Graphics& g, juce::Rectangle<int> bounds)
{
	g.setColour (juce::Colours::bisque);
	g.setFont (14.0f);
	juce::String const s = juce::String(ProjectInfo::companyName) +
		juce::String(" ") +
		juce::String(ProjectInfo::projectName) +
		juce::String(" version ") +
		juce::String(ProjectInfo::versionString) +
		juce::String("     ");
	g.drawText (s, bounds, juce::Justification::bottomRight, true);
}

class Slicer_granularAudioProcessorEditor  : 	public juce::AudioProcessorEditor
,												public GranularEditorCommon
{
public:
    Slicer_granularAudioProcessorEditor (SlicerGranularAudioProcessor&);
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
private:	
	std::array<juce::Colour, 5> gradientColors {
		juce::Colours::darkred,
		juce::Colours::darkred,
		juce::Colours::red,
		juce::Colours::darkred,
		juce::Colours::black
	};
	size_t colourOffsetIndex {0};
	
    SlicerGranularAudioProcessor& audioProcessor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Slicer_granularAudioProcessorEditor)
};

inline 
int placeFileCompAndGrainBusyDisplay(juce::Rectangle<int> localBounds, int pad, GrainBusyDisplay &grainBusyDisplay, FileSelectorComponent &fileComp, int y) {
	int const fileCompAndGrainDisplayHeight = 26;
	{
		int const pad = 2;
		int const grainDisplayHeight = fileCompAndGrainDisplayHeight - pad;
		
		grainBusyDisplay.setSizePerGrain((float)grainDisplayHeight / (float)N_VOICES);
		float const sizePerGrain = grainBusyDisplay.getSizePerGrain();
		
		int const grainBusyDisplayWidth = (float)N_GRAINS * sizePerGrain - pad;
		
		int const grainBusyX = localBounds.getX() + (localBounds.getWidth() - grainBusyDisplayWidth) + pad/2;
		int const grainBusyY = y + pad/2;
		grainBusyDisplay.setBounds(grainBusyX, grainBusyY, grainBusyDisplayWidth, grainDisplayHeight);
	}
	{
		int const fileCompWidth = localBounds.getWidth() - grainBusyDisplay.getWidth();
		int const x(localBounds.getX());
		fileComp.setBounds(x, y, fileCompWidth, fileCompAndGrainDisplayHeight);
		y += fileCompAndGrainDisplayHeight;
		y += pad;
	}
	return y;	// needs to know the new y to place components at
}
