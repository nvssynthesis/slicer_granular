/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
 
#include "SlicerGranularPluginProcessor.h"
#include "SlicerGranularPluginEditor.h"

GranularEditorCommon::GranularEditorCommon (SlicerGranularAudioProcessor& p)
:	waveformAndPositionComponent(p)
,	presetPanel(p.getPresetManager())
,	tabbedPages(p.getAPVTS())
,	audioProcessor(p)
{
	audioProcessor.addSampleManagementGutsListener(this);
	audioProcessor.addMeasuredGrainDescriptionsListener(this);
	
	auto const fileToRead = audioProcessor.getSampleFilePath();
	drawThumbnail();
}
GranularEditorCommon::~GranularEditorCommon() {
	audioProcessor.removeSampleManagementGutsListener(this);
	audioProcessor.removeMeasuredGrainDescriptionsListener(this);
}

void GranularEditorCommon::drawThumbnail(){
	auto const &synthBuffer = audioProcessor.viewSynthSharedState()._buffer;

	if (sampleManagementGuts == nullptr){
		return;
	}
	jassert (0 < sampleManagementGuts->getLength());
	jassert (0 < sampleManagementGuts->getNumChannels());
	jassert (synthBuffer._file_sample_rate > 0);

	waveformAndPositionComponent.wc.setThumbnailSource(&sampleManagementGuts->getSampleBuffer(),	// do not worry about dangling reference; the thumbnail will internally copy the data as needed to draw waveform
													   synthBuffer._file_sample_rate, synthBuffer._filename_hash);
}
//============================================= ChangeListener - related =======================================================
void GranularEditorCommon::displayGrainDescriptions() {
	audioProcessor.readGrainDescriptionData(grainDescriptions);
	waveformAndPositionComponent.wc.removeMarkers(WaveformComponent::MarkerType::CurrentPosition);
	for (auto gd : grainDescriptions){
		waveformAndPositionComponent.wc.addMarker(gd);
		grainBusyDisplay.setStatus(gd.grain_id, gd.voice, gd.busy);
		grainBusyDisplay.repaint();
	}
}
void GranularEditorCommon::handleGrainDescriptionBroadcast(){
	displayGrainDescriptions();
	waveformAndPositionComponent.wc.repaint();
}
void GranularEditorCommon::handleSampleManagementBroadcast(){
	audioProcessor.writeToLog("common: handling sample management broadcast");
	
	auto const fileToRead = audioProcessor.getSampleFilePath();
	drawThumbnail();
}
void GranularEditorCommon::changeListenerCallback (juce::ChangeBroadcaster* source){
	if (auto *smg = dynamic_cast<nvs::util::SampleManagementGuts*>(source)){	// used to be received asynchronously
		sampleManagementGuts = smg;
		handleSampleManagementBroadcast();
	}
	else if (dynamic_cast<nvs::util::MeasuredData*>(source)) {
		handleGrainDescriptionBroadcast();
	}
	else {
		audioProcessor.writeToLog("no match for listener.\n");
	}
}
//==============================================================================
Slicer_granularAudioProcessorEditor::Slicer_granularAudioProcessorEditor (SlicerGranularAudioProcessor& p)
    : AudioProcessorEditor (&p)
,	GranularEditorCommon(p)
,	audioProcessor (p)
{
	addAndMakeVisible (presetPanel);
	addAndMakeVisible(tabbedPages);
	addAndMakeVisible(waveformAndPositionComponent);
	addAndMakeVisible(grainBusyDisplay);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
	getConstrainer()->setMinimumSize(300, 420);

    setSize (800, 500);
	setResizable(true, true);
}
//==============================================================================
void Slicer_granularAudioProcessorEditor::paint (juce::Graphics& g)
{
	juce::Image image(juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics tg(image);

	juce::Colour upperLeftColour  = gradientColors[(colourOffsetIndex + 0) % gradientColors.size()];
	juce::Colour lowerRightColour = gradientColors[(colourOffsetIndex + 4) % gradientColors.size()];
	juce::ColourGradient cg(upperLeftColour, 0, 0, lowerRightColour, getWidth(), getHeight(), true);
	cg.addColour(0.3, gradientColors[(colourOffsetIndex + 1) % gradientColors.size()]);
	cg.addColour(0.5, gradientColors[(colourOffsetIndex + 2) % gradientColors.size()]);
	cg.addColour(0.7, gradientColors[(colourOffsetIndex + 3) % gradientColors.size()]);
    tg.setGradientFill(cg);
    tg.fillAll();

	auto const b = getLocalBounds();
    g.drawImage(image, b.toFloat());
	
	displayName(g, b);
}

void Slicer_granularAudioProcessorEditor::resized()
{
	juce::Rectangle<int> localBounds = getLocalBounds();
	int const smallPad = 10;
	localBounds.reduce(smallPad, smallPad);
	
	int y(localBounds.getY());
	y = placeFileCompAndGrainBusyDisplay(localBounds, smallPad, grainBusyDisplay, presetPanel, y);
	{
		auto const mainParamsRemainingHeightRatio = 0.8 * localBounds.getHeight();

		int const alottedMainParamsHeight = mainParamsRemainingHeightRatio - y + smallPad;
		int const alottedMainParamsWidth = localBounds.getWidth();
		
		tabbedPages.setBounds(localBounds.getX(), y, alottedMainParamsWidth, alottedMainParamsHeight);
		y += tabbedPages.getHeight();
	}
	auto const remainingHeight = 0.2f * localBounds.getHeight();
	waveformAndPositionComponent.setBounds(localBounds.getX(), y, localBounds.getWidth(), remainingHeight);
}
