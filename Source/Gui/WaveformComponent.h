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
	
	void drawMarker(juce::Graphics& g, double pos){
		
		g.setColour(juce::Colours::blue);
		
		int xPos = getWidth() * pos;
		
		g.drawLine(xPos, getY(), xPos, getBottom(), 2.f);
	}
	
	void paint(juce::Graphics& g) override
	{
		g.setColour (juce::Colours::white);
		auto const bounds = getLocalBounds();
		g.drawRect(bounds);
		
		if (thumbnail.getNumChannels() == 0) {
			paintContentsIfNoFileLoaded (g);
		}
		else {
			paintContentsIfFileLoaded (g);
		}
		
		for (auto i = 0; i < markerList.getNumMarkers(); ++i){
			double pos = markerList.getMarkerPosition(*markerList.getMarker(i), this);
			drawMarker(g, pos);
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
	void paintContentsIfNoFileLoaded (juce::Graphics& g)
	{
		g.setColour (juce::Colours::darkgrey);
		g.drawFittedText ("No File Loaded", getLocalBounds(), juce::Justification::centred, 1);
	}
	void paintContentsIfFileLoaded (juce::Graphics& g)
	{
		g.setColour (juce::Colours::black);
		thumbnail.drawChannels (g,
								getLocalBounds(),
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
		addAndMakeVisible(wc);
		addAndMakeVisible(&positionSlider._slider);
	}
	
	void resized() override
	{
		auto const localBounds = getLocalBounds();
		
		auto const totalHeight = localBounds.getHeight();
		auto const reservedHeight = totalHeight * 0.9;

		juce::Rectangle<int> const wcRect = [&]{	// limit scope via instantly-called lambda
			auto const heightDiff = totalHeight - reservedHeight;
			auto const waveformY = localBounds.getY() + (heightDiff * 0.5);
			auto const waveformHeight = reservedHeight * 0.8f;
			
			auto const waveformWidth = localBounds.getWidth() * 1.0;
			auto const waveformWidthDiff = localBounds.getWidth() - waveformWidth;
			auto const waveformX = localBounds.getX() + (waveformWidthDiff * 0.5);

			auto const wcRect = juce::Rectangle<int>(waveformX, waveformY, waveformWidth, waveformHeight);
			wc.setBounds(wcRect);
			return wcRect;	// return rectangle as that's all we need from this scope
		}();
		
		{	// limit scope for drawing slider
			auto const sliderHeight = reservedHeight - wcRect.getHeight();
			int const widthIncrease = 14;
			auto const sliderWidth = wcRect.getWidth() + widthIncrease;
			auto const sliderX = wcRect.getX() - (widthIncrease*0.5);
			auto const sliderY = wcRect.getBottom();
			auto const sliderRect = juce::Rectangle<int>(sliderX, sliderY, sliderWidth, sliderHeight);
			positionSlider._slider.setBounds(sliderRect);
		}
	}
	void paint (juce::Graphics& g) override {
		
	}
	
	void sliderValueChanged (juce::Slider *slider) override {
		if (slider == &positionSlider._slider){
			// this used to set the position of the read-only slider
		}
	}

	// make this accessible from outside so there's no need for a bunch of forwarding methods
	WaveformComponent wc;
private:
	AttachedSlider positionSlider;
	std::atomic<double> position;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAndPositionComponent);
};
