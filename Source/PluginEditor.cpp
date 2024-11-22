/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/
 
#include "PluginProcessor.h"
#include "PluginEditor.h"
//==============================================================================

Slicer_granularAudioProcessorEditor::Slicer_granularAudioProcessorEditor (Slicer_granularAudioProcessor& p)
    : AudioProcessorEditor (&p)
,	fileComp(juce::File(), "*.wav;*.aif;*.aiff", "", "Select file to open", false)
,	tabbedPages(p.apvts)
,	waveformAndPositionComponent(512, p.getAudioFormatManager(), p.apvts)
,	audioProcessor (p)
{
	addAndMakeVisible (fileComp);
	fileComp.addListener (this);
	fileComp.getRecentFilesFromUserApplicationDataDirectory();
	
	addAndMakeVisible(tabbedPages);
	
	addAndMakeVisible(waveformAndPositionComponent);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
	constrainer.setMinimumSize(600, 480);

    setSize (800, 500);
	setResizable(true, true);
}

Slicer_granularAudioProcessorEditor::~Slicer_granularAudioProcessorEditor()
{
	fileComp.pushRecentFilesToFile();
}
//==============================================================================
void Slicer_granularAudioProcessorEditor::updateToggleState (juce::Button* button, juce::String name, bool &valToAffect)
{
	bool state = button->getToggleState();
	valToAffect = state;
	juce::String stateString = state ? "ON" : "OFF";

	juce::Logger::outputDebugString (name + " Button changed to " + stateString);
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

    g.drawImage(image, getLocalBounds().toFloat());

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
}

void Slicer_granularAudioProcessorEditor::resized()
{
	constrainer.checkComponentBounds(this);
	juce::Rectangle<int> localBounds = getLocalBounds();
	int const smallPad = 10;
	localBounds.reduce(smallPad, smallPad);
	
	int x(0), y(0);
	{	// just some scopes for temporaries
		int fileCompWidth = localBounds.getWidth();
		int fileCompHeight = 20;
		x = localBounds.getX();
		y = localBounds.getY();
		fileComp.setBounds(x, y, fileCompWidth, fileCompHeight);
		y += fileCompHeight;
		y += smallPad;
	}
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

void Slicer_granularAudioProcessorEditor::readFile (const juce::File& fileToRead)
{
	if (fileToRead.isDirectory()){
		// handle whole directory
	}
	if (! fileToRead.existsAsFile())
		return;

	std::string fn = fileToRead.getFullPathName().toStdString();
	
	audioProcessor.writeToLog(fn);
	audioProcessor.loadAudioFile(fileToRead, waveformAndPositionComponent.wc.getThumbnail() );

	fileComp.setCurrentFile(fileToRead, true);
}

void Slicer_granularAudioProcessorEditor::filenameComponentChanged (juce::FilenameComponent* fileComponentThatHasChanged)
{
	if (fileComponentThatHasChanged == &fileComp){
		readFile (fileComp.getCurrentFile());
	}
}
