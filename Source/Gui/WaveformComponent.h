/*
  ==============================================================================

    WaveformComponent.h
    Created: 16 Sep 2023 8:08:39am
    Author:  Nicholas Solem

  ==============================================================================
*/

/**
TODO:
    Remove more aspects relevant only to TSN, such as onset markers.
    These should just be handled by the derived class, SegmentedWaveformComponent.
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "AttachedSlider.h"
#include "../Synthesis/GrainDescription.h"
#include "../utils/misc_util.h"
#include "IndexTypes.h"

class SlicerGranularAudioProcessor;

class WaveformComponent		:	public Component
,								public ChangeListener
,								public FileDragAndDropTarget
{
public:
    explicit WaveformComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample=512);

	enum class MarkerType {
		Onset = 0,
		CurrentPosition
	};
	size_t getNumMarkers(MarkerType markerType) const;
	std::vector<double> getNormalizedOnsets() const;
	void addMarker(double onsetPosition);							        // adds an OnsetMarker
	void addMarker(nvs::gran::GrainDescription const &grainDescription);	// adds a PositionMarker
	void removeMarkers(MarkerType markerType);

    //============================================================================================================
	void paint(Graphics& g) override;
	void resized() override;
    //============================================================================================================
	void changeListenerCallback (ChangeBroadcaster* source) override;
    //============================================================================================================
	virtual void setThumbnailSource (const AudioBuffer<float> *newSource, double sampleRate, int64 hashCode);
    int64 getHashCode() const;
	void highlightOnsets(std::vector<nvs::timbrespace::WeightedIdx> const &currentIndices);
	//============================================================================================================
	void mouseUp(MouseEvent const &e) override;
	//============================================================================================================
	bool isInterestedInFileDrag (StringArray const& files) override;
	void filesDropped (StringArray const& files, int x, int y) override;
	void fileDragEnter (StringArray const& files, int x, int y) override;
	void fileDragExit (StringArray const& files) override;
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
protected:
	Rectangle<int> waveformBounds;
private:
	SlicerGranularAudioProcessor &_proc;
	AudioThumbnailCache thumbnailCache;
	AudioThumbnail thumbnail;
	bool isDragOver { false };
	
	std::vector<OnsetMarker> onsetMarkerList;
	std::vector<PositionMarker> currentPositionMarkerList;
	using MarkerListVariant = std::variant<std::vector<OnsetMarker>*, std::vector<PositionMarker>*>;
	std::map<MarkerType, MarkerListVariant> markerListMap {
		{MarkerType::Onset, MarkerListVariant(&onsetMarkerList)},
		{MarkerType::CurrentPosition, MarkerListVariant(&currentPositionMarkerList)}
	};
	void drawMarkers(Graphics& g, MarkerType markerType);
	using MarkerVariant = std::variant<OnsetMarker, PositionMarker>;
	void drawMarker(Graphics& g, MarkerVariant marker);

	void thumbnailChanged();
	
	std::optional<std::vector<std::pair<double, double>>> highlightedRange;
	
	void paintContentsIfNoFileLoaded (Graphics& g);
	void paintContentsIfFileLoaded (Graphics& g);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

class WaveformAndPositionComponent	:	public WaveformComponent
{
public:
    explicit WaveformAndPositionComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample=512);
	
	void resized() override;
	// void paint (Graphics& g) override;
	
	void hideSlider();	// effectively makes it function as just the waveformComponent. I don't want to simply use that though because then the slicer_granular version has to change a bunch of code based on #ifdef TSN.
	
	//========================================================================================

private:
	AttachedSlider positionSlider;
	std::atomic<double> position;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAndPositionComponent)
};
