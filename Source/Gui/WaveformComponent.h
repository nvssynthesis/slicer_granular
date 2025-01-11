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
{
public:
	WaveformComponent(int sourceSamplesPerThumbnailSample, juce::AudioFormatManager &formatManagerToUse);
	
	enum class MarkerType {
		Onset = 0,
		CurrentPosition
	};
	int getNumMarkers(MarkerType markerType);
	void addMarker(double pos, MarkerType markerType);
	void removeMarkers(MarkerType markerType);
	void drawMarker(juce::Graphics& g, double pos, MarkerType markerType);
	void drawMarkers(juce::Graphics& g, MarkerType markerType);

	void paint(juce::Graphics& g) override;
	void resized() override {}
	void changeListenerCallback (juce::ChangeBroadcaster* source) override;
	
	juce::AudioThumbnail *const getThumbnail();
private:
	juce::AudioThumbnailCache thumbnailCache;
	juce::AudioThumbnail thumbnail;
	
	juce::MarkerList onsetMarkerList;
	juce::MarkerList currentPositionMarkerList;
	std::map<MarkerType, juce::MarkerList &> markerListMap {
		{MarkerType::Onset, onsetMarkerList},
		{MarkerType::CurrentPosition, currentPositionMarkerList}
	};
	
	void thumbnailChanged()
	{
		repaint();
	}
	void paintContentsIfNoFileLoaded (juce::Graphics& g);
	void paintContentsIfFileLoaded (juce::Graphics& g);
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
	WaveformAndPositionComponent(int sourceSamplesPerThumbnailSample,
								 juce::AudioFormatManager &formatManagerToUse,
								 juce::AudioProcessorValueTreeState &apvts);
	
	void resized() override;
	void paint (juce::Graphics& g) override;
	
	void sliderValueChanged (juce::Slider *slider) override;

	WaveformComponent wc; // externally accessible 
private:
	AttachedSlider positionSlider;
	std::atomic<double> position;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAndPositionComponent);
};
