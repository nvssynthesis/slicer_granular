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
	void resized() override
	{
		
	}
	void changeListenerCallback (juce::ChangeBroadcaster* source) override
	{
		if (source == &thumbnail)
			thumbnailChanged();
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
