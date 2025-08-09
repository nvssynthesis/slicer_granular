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
#include "../Synthesis/GrainDescription.h"
#include "../misc_util.h"

class SlicerGranularAudioProcessor;


class WaveformComponent		:	public juce::Component
,								public juce::ChangeListener
,								public juce::FileDragAndDropTarget
{
public:
    explicit WaveformComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample=512);
	
	enum class MarkerType {
		Onset = 0,
		CurrentPosition
	};
	size_t getNumMarkers(MarkerType markerType);
	std::vector<double> getNormalizedOnsets() const;
	void addMarker(double onsetPosition);							// adds an OnsetMarker
	void addMarker(nvs::gran::GrainDescription const &grainDescription);	// adds a PositionMarker
	void removeMarkers(MarkerType markerType);
    //============================================================================================================
	void paint(juce::Graphics& g) override;
	void resized() override {}
    //============================================================================================================
	void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    //============================================================================================================
	void setThumbnailSource (const juce::AudioBuffer<float> *newSource, double sampleRate, juce::int64 hashCode);
	void highlightOnsets(std::vector<nvs::util::WeightedIdx> const &currentIndices);
	//============================================================================================================
	void mouseUp(juce::MouseEvent const &e) override;
	//============================================================================================================
	bool isInterestedInFileDrag (juce::StringArray const& files) override;
	void filesDropped (juce::StringArray const& files, int x, int y) override;
	void fileDragEnter (juce::StringArray const& files, int x, int y) override;
	void fileDragExit (juce::StringArray const& files) override;
	//============================================================================================================
	struct OnsetMarker {
		double position;
	};
	struct PositionMarker {
		int grain_id;
		double position;
		double sample_playback_rate;
		float window;
		float pan;
		bool busy;
		bool first_playthrough;
		static PositionMarker fromGrainDescription(nvs::gran::GrainDescription const &gd){
			return PositionMarker{gd.grain_id, gd.position, gd.sample_playback_rate, gd.window, gd.pan, gd.busy, gd.first_playthrough};
		}
	};
private:
	SlicerGranularAudioProcessor &_proc;
	juce::AudioThumbnailCache thumbnailCache;
	juce::AudioThumbnail thumbnail;
	bool isDragOver { false };
	
	std::vector<OnsetMarker> onsetMarkerList;
	std::vector<PositionMarker> currentPositionMarkerList;
	using MarkerListVariant = std::variant<std::vector<OnsetMarker>*, std::vector<PositionMarker>*>;
	std::map<MarkerType, MarkerListVariant> markerListMap {
		{MarkerType::Onset, MarkerListVariant(&onsetMarkerList)},
		{MarkerType::CurrentPosition, MarkerListVariant(&currentPositionMarkerList)}
	};
	void drawMarkers(juce::Graphics& g, MarkerType markerType);
	using MarkerVariant = std::variant<OnsetMarker, PositionMarker>;
	void drawMarker(juce::Graphics& g, MarkerVariant marker);

	void thumbnailChanged();
	
	std::optional<std::vector<std::pair<double, double>>> highlightedRange;
	
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
{
public:
    explicit WaveformAndPositionComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample=512);
	
	void resized() override;

	
	void hideSlider();	// effectively makes it function as just the waveformComponent. I don't want to simply use that though because then the slicer_granular version has to change a bunch of code based on #ifdef TSN.
	
	//========================================================================================

	WaveformComponent wc; // externally accessible 
private:
	AttachedSlider positionSlider;
	std::atomic<double> position;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAndPositionComponent);
};
