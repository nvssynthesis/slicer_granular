/*
  ==============================================================================

    WaveformComponent.h
    Created: 16 Sep 2023 8:08:39am
    Author:  Nicholas Solem

  ==============================================================================
*/

/**
-instantiate in PluginProcessor
 
 when processor.loadAudioFile() is called, need to use
	 WaveformComponent.thumbnail.setSource (new juce::FileInputSource (file));

 */

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "AttachedSlider.h"
#include "fmt/core.h"

class WaveformComponent		:	public juce::Component
,								private juce::ChangeListener
//,								private juce::MarkerList::Listener
{
public:
	WaveformComponent(int sourceSamplesPerThumbnailSample, juce::AudioFormatManager &formatManagerToUse)
	:	thumbnailCache(5), thumbnail(sourceSamplesPerThumbnailSample, formatManagerToUse, thumbnailCache)
	{
		thumbnail.addChangeListener(this);	// thumbnail is a ChangeBroadcaster
	}
	
	int getNumMarkers(){
		return markerList.getNumMarkers();
	}
	void addMarker(double pos){
		juce::String name(pos);
		markerList.setMarker(name, juce::RelativeCoordinate(pos));
	}
	void removeMarkers(){
		auto numMarkers = markerList.getNumMarkers();
		for (auto i = numMarkers - 1; i >= 0 ; --i){
			markerList.removeMarker(i);
		}
	}
	
	void drawMarker(juce::Graphics& g, juce::Rectangle<int> bounds, double pos){
		
		g.setColour(juce::Colours::blue);
		
		int xPos = (bounds.getWidth() * pos) + bounds.getX();
		
		g.drawLine(xPos, bounds.getY(), xPos, bounds.getBottom(), 2.f);
	}
	
	void paint(juce::Graphics& g) override
	{
		juce::Rectangle<int> thumbnailBounds (10, 10, getWidth() - 20, getHeight() - 20);

		if (thumbnail.getNumChannels() == 0)
			paintIfNoFileLoaded (g, thumbnailBounds);
		else
			paintIfFileLoaded (g, thumbnailBounds);
		
		for (auto i = 0; i < markerList.getNumMarkers(); ++i){
			double pos = markerList.getMarkerPosition(*markerList.getMarker(i), this);
			drawMarker(g, thumbnailBounds, pos);
		}
	}
	void resized() override {}
	void changeListenerCallback (juce::ChangeBroadcaster* source) override
	{
		if (source == &thumbnail){
			thumbnailChanged();
		}
	}
	
	juce::AudioThumbnail *const getThumbnail(){
	/*
	 not sure if it's better to:
	 -	use this to expose thumbnail in order to setSource in processor (such getters are a code smell), or
	 -	just make a public setSource method in WaveformComponent to call from AudioProcessor. disadvantage: have to #include the WaveformComponent in AudioProcessor, which feels wrong because its basically a GUI element.
	 */
		return &thumbnail;
	}
	
private:
	juce::AudioThumbnailCache thumbnailCache;
	juce::AudioThumbnail thumbnail;
	
	juce::MarkerList markerList;
	
	void thumbnailChanged()
	{
		repaint();
	}
	void paintIfNoFileLoaded (juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
	{
		g.setColour (juce::Colours::darkgrey);
		g.drawRect(thumbnailBounds);
		g.drawFittedText ("No File Loaded", thumbnailBounds, juce::Justification::centred, 1);
	}
	void paintIfFileLoaded (juce::Graphics& g, const juce::Rectangle<int>& thumbnailBounds)
	{
		g.setColour (juce::Colours::darkgrey);
		g.drawRect(thumbnailBounds);
		
		g.setColour (juce::Colours::black);

		thumbnail.drawChannels (g,
								thumbnailBounds,
								0.0,                                    // start time
								thumbnail.getTotalLength(),             // end time
								1.0f);                                  // vertical zoom
	}
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent);
};

/**
 The trick to implement:
 Levels of position quantization
 In the editor, user can set 'requested' position via Position slider.
 These potentially get quantized based on onsetsInSeconds (held in the Processor).
 The positionQuantizedReadOnlySlider should follow that quantized value
 */
class WaveformAndPositionComponent	:	public juce::Component
,										public juce::Slider::Listener
{
public:
	WaveformAndPositionComponent(int sourceSamplesPerThumbnailSample, juce::AudioFormatManager &formatManagerToUse,
								 juce::AudioProcessorValueTreeState &apvts)
	:	wc(sourceSamplesPerThumbnailSample, formatManagerToUse)
	,	positionSlider(apvts, params_e::position, juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::NoTextBox)
	{
		positionSlider._slider.addListener(this);
		
		addAndMakeVisible(positionQuantizedReadOnlySlider);
		positionQuantizedReadOnlySlider.setRange(0.0, 1.0);
		positionQuantizedReadOnlySlider.setValue(0.0);
		positionQuantizedReadOnlySlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 100, 100);
		positionQuantizedReadOnlySlider.setEnabled(false);
		positionQuantizedReadOnlySlider.setColour(juce::Slider::ColourIds::thumbColourId, juce::Colours::white);
		addAndMakeVisible(wc);
		addAndMakeVisible(&positionSlider._slider);
	}
	
	void resized() override
	{
		auto const localBounds = getLocalBounds();
		
		auto const totalHeight = localBounds.getHeight();
		auto const waveformHeight = totalHeight * 0.8f;
		auto const sliderHeight = totalHeight - waveformHeight;
		
		wc.setBounds(localBounds.getX(), localBounds.getY(), localBounds.getWidth(), waveformHeight);
		positionSlider._slider.setBounds(localBounds.getX(), wc.getBottom(), localBounds.getWidth(), sliderHeight);
		positionQuantizedReadOnlySlider.setBounds(positionSlider._slider.getX(), positionSlider._slider.getY(), positionSlider._slider.getWidth(), positionSlider._slider.getHeight());
	}
	void paint (juce::Graphics& g) override {}
	
	void sliderValueChanged (juce::Slider *slider) override {
		if (slider == &positionSlider._slider){
			positionQuantizedReadOnlySlider.setValue(positionSlider._slider.getValue());
		}
	}

	// make this accessible from outside so there's no need for a bunch of forwarding methods
	WaveformComponent wc;
private:
	AttachedSlider positionSlider;
	juce::Slider positionQuantizedReadOnlySlider;
	std::atomic<double> position;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAndPositionComponent);
};
