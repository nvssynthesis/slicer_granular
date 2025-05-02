/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
 
#include "SlicerGranularPluginProcessor.h"
#include "SlicerGranularPluginEditor.h"

GranularEditorCommon::GranularEditorCommon (Slicer_granularAudioProcessor& p)
:	waveformAndPositionComponent(512, p.getAudioFormatManager(), p.getAPVTS())
,	fileComp(p.getSampleFilePath(), "*.wav;*.aif;*.aiff", "", "Select file to open", false)
,	tabbedPages(p.getAPVTS())
,	audioProcessor(p)
{
	audioProcessor.addSampleManagementGutsListener(this);
	audioProcessor.addMeasuredGrainDescriptionsListener(this);
	
	fileComp.setCurrentFile(audioProcessor.getSampleFilePath(), false);
	auto const fileToRead = audioProcessor.getSampleFilePath();
	notateFileComp(fileToRead);
	drawThumbnail(fileToRead);
	startTimer(100);
	
	fileComp.addListener (this);
	fileComp.getRecentFilesFromUserApplicationDataDirectory();
}
GranularEditorCommon::~GranularEditorCommon() {
	audioProcessor.removeSampleManagementGutsListener(this);
	audioProcessor.removeMeasuredGrainDescriptionsListener(this);
	fileComp.pushRecentFilesToFile();
}

void GranularEditorCommon::readFile (const juce::File& fileToRead)
{
	audioProcessor.writeToLog("Slicer_granularAudioProcessorEditor::readFile, reading " + fileToRead.getFullPathName());

	if (fileToRead.isDirectory()){ /* handle whole directory */ }
	if (! fileToRead.existsAsFile()){
		audioProcessor.writeToLog("... in readFile, file does NOT exist as a single file. ");
		return;
	}
	
	audioProcessor.writeToLog("... in readFile, file exists as a single file. ");

	juce::String fn = fileToRead.getFullPathName();

	audioProcessor.writeToLog("fileToRead: " + fn);
	audioProcessor.loadAudioFile(fileToRead, false);
	notateFileComp(fn);
	drawThumbnail(fn);
}

void GranularEditorCommon::notateFileComp(juce::String const &sampleFilePath){
	audioProcessor.writeToLog("Notating fileComp with " + sampleFilePath);
	fileComp.setCurrentFile(sampleFilePath, true, juce::dontSendNotification);
}
void GranularEditorCommon::drawThumbnail(juce::String const &sampleFilePath){
	auto thumbnail = waveformAndPositionComponent.wc.getThumbnail();
	if (thumbnail){
		thumbnail->setSource (new juce::FileInputSource (sampleFilePath));	// owned by thumbnail, no worry about delete
	}
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
	notateFileComp(fileToRead);
	drawThumbnail(fileToRead);
}
void GranularEditorCommon::changeListenerCallback (juce::ChangeBroadcaster* source){
	if (dynamic_cast<nvs::util::SampleManagementGuts*>(source)){
		handleSampleManagementBroadcast();
	}
	else if (dynamic_cast<nvs::util::MeasuredData*>(source)) {
		handleGrainDescriptionBroadcast();
	}
	else {
		audioProcessor.writeToLog("no match for listener.\n");
	}
}
//==========================================================  Timer  ============================================================
void GranularEditorCommon::timerCallback() {
	// this is a workaround for now to address the behavior of the file comp notating the DAW's path
	audioProcessor.writeToLog("Timer callback notating file comp");
	notateFileComp(audioProcessor.getSampleFilePath());
	stopTimer();
}
//=================================================  FilenameComponentListener  =================================================
void GranularEditorCommon::filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged)
{
	audioProcessor.writeToLog("editor: filenameComponentChanged.");
	if (fileComponentThatHasChanged == &fileComp){
		const juce::File file = fileComp.getCurrentFile();
		audioProcessor.writeToLog("     filenameComponentChanged: source is fileComp. triggering readFile with " + file.getFullPathName());
		if (file.existsAsFile()) {
			audioProcessor.writeToLog(file.getFullPathName() + " exists as file.");
			audioProcessor.loadAudioFile(file, false);  // Ensure this is thread-safe
			auto const fileToRead = audioProcessor.getSampleFilePath();
			notateFileComp(fileToRead);
			drawThumbnail(fileToRead);
		}
	}
}
//==============================================================================
Slicer_granularAudioProcessorEditor::Slicer_granularAudioProcessorEditor (Slicer_granularAudioProcessor& p)
    : AudioProcessorEditor (&p)
,	GranularEditorCommon(p)
,	audioProcessor (p)
{
	addAndMakeVisible (fileComp);
	addAndMakeVisible(tabbedPages);
	addAndMakeVisible(waveformAndPositionComponent);
	addAndMakeVisible(grainBusyDisplay);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
	constrainer.setMinimumSize(600, 480);

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
	constrainer.checkComponentBounds(this);
	juce::Rectangle<int> localBounds = getLocalBounds();
	int const smallPad = 10;
	localBounds.reduce(smallPad, smallPad);
	
	int y(localBounds.getY());
	y = placeFileCompAndGrainBusyDisplay(localBounds, smallPad, grainBusyDisplay, fileComp, y);
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
