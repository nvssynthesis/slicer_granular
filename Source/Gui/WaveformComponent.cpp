/*
  ==============================================================================

    WaveformComponent.cpp
    Created: 9 Jan 2025 3:41:31pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "WaveformComponent.h"


WaveformComponent::WaveformComponent(int sourceSamplesPerThumbnailSample, juce::AudioFormatManager &formatManagerToUse)
:	thumbnailCache(5), thumbnail(sourceSamplesPerThumbnailSample, formatManagerToUse, thumbnailCache)
{
	thumbnail.addChangeListener(this);	// thumbnail is a ChangeBroadcaster
}
	
size_t WaveformComponent::getNumMarkers(MarkerType markerType) {
	const auto& markerListVariant = markerListMap.at(markerType);
	return std::visit([](auto* markerList) { return markerList->size(); }, markerListVariant);
}
void WaveformComponent::addMarker(double onsetPosition) {
	auto it = std::lower_bound(onsetMarkerList.begin(), onsetMarkerList.end(), onsetPosition,
							   [](const OnsetMarker& marker, double position) {
								   return marker.position < position;
							   });

	onsetMarkerList.insert(it, OnsetMarker{onsetPosition});
}
void WaveformComponent::addMarker(nvs::gran::GrainDescription gd){
	auto it = std::lower_bound(currentPositionMarkerList.begin(), currentPositionMarkerList.end(), gd,
							   [](const PositionMarker& marker, nvs::gran::GrainDescription gd) {
									return marker.position < gd.position;
							   });
	currentPositionMarkerList.insert(it, PositionMarker::fromGrainDescription(gd));
}
void WaveformComponent::removeMarkers(MarkerType markerType) {
	auto& markerListVariant = markerListMap.at(markerType);

	std::visit([](auto* markerList) {
		markerList->clear();
		assert(markerList->empty());
	}, markerListVariant);
}
void WaveformComponent::drawMarkers(juce::Graphics& g, MarkerType markerType){
	auto const &markerListVariant = markerListMap.at(markerType);
	std::visit([&](auto const& markerList) {
		for (auto const& marker : *markerList) {
			drawMarker(g, marker);
		}
	}, markerListVariant);
}
void WaveformComponent::drawMarker(juce::Graphics& g, MarkerVariant marker)
{
	auto const line = [&]
	{
		double position = std::visit([](const auto& marker) {
			return marker.position; // position is a common member to all alternatives
		}, marker);
		float const xPos = getWidth() * position;
		float y0 = getLocalBounds().getY();
		float y1 = getLocalBounds().getBottom();
		auto l = juce::Line<float>(juce::Point<float>{xPos, y0}, juce::Point<float>{xPos, y1});
		std::visit([&](const auto &marker) {
			if constexpr (std::is_same_v<std::decay_t<decltype(marker)>, OnsetMarker>) {
				g.setColour(juce::Colours::blue);
			} else if constexpr (std::is_same_v<std::decay_t<decltype(marker)>, PositionMarker>) {
				g.setColour(juce::Colours::lightgreen);
				[[maybe_unused]] auto p = (marker.pan * 2.0) - 1.0;					// affect y-center position
				[[maybe_unused]] auto r = marker.sample_playback_rate;
				[[maybe_unused]] auto w = marker.window;
				g.setOpacity(sqrt(w));
				auto const shortenBy = l.getLength() * 0.25;
				l = l.withShortenedStart(shortenBy);
				l = l.withShortenedEnd(shortenBy);
				l.applyTransform(juce::AffineTransform::translation(0.0f, p * shortenBy));
//				juce::Point<float> start = transform.translation(l.getStartX(), l.getStartY());
//				l = juce::Line<float>(start, juce::Point<float>(l.getEnd()));
			}
		}, marker);
		return l;
	}();
	
	g.drawLine(line, 1.f);
}
void WaveformComponent::paint(juce::Graphics& g)
{
	g.setColour (juce::Colours::darkgrey);
	auto const bounds = getLocalBounds();
	g.drawRect(bounds);
	
	if (thumbnail.getNumChannels() == 0) {
		paintContentsIfNoFileLoaded (g);
	}
	else {
		paintContentsIfFileLoaded (g);
	}
	drawMarkers(g, MarkerType::Onset);
	drawMarkers(g, MarkerType::CurrentPosition);
}

void WaveformComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
	if (source == &thumbnail){
		thumbnailChanged();
	}
}
	
juce::AudioThumbnail *const WaveformComponent::getThumbnail(){
/*
 not sure if it's better to:
 -	use this to expose thumbnail in order to setSource in processor (such getters are a code smell), or
 -	just make a public setSource method in WaveformComponent to call from AudioProcessor. disadvantage: have to #include the WaveformComponent in AudioProcessor, which feels wrong because its basically a GUI element.
 */
	return &thumbnail;
}

void WaveformComponent::paintContentsIfNoFileLoaded (juce::Graphics& g)
{
	g.setColour (juce::Colours::darkgrey);
	g.drawFittedText ("No File Loaded", getLocalBounds(), juce::Justification::centred, 1);
}
void WaveformComponent::paintContentsIfFileLoaded (juce::Graphics& g)
{
	g.setColour (juce::Colours::black);
	thumbnail.drawChannels (g,
							getLocalBounds(),
							0.0,                                    // start time
							thumbnail.getTotalLength(),             // end time
							1.0f);                                  // vertical zoom
}

//================================================================================================================================================
/**
 The trick to implement:
 Levels of position quantization
 In the editor, user can set 'requested' position via Position slider.
 These potentially get quantized based on onsetsInSeconds (held in the Processor).
 The positionQuantizedReadOnlySlider should follow that quantized value
 */

WaveformAndPositionComponent::WaveformAndPositionComponent(int sourceSamplesPerThumbnailSample, juce::AudioFormatManager &formatManagerToUse,
								 juce::AudioProcessorValueTreeState &apvts)
:	wc(sourceSamplesPerThumbnailSample, formatManagerToUse)
,	positionSlider(apvts, params_e::position, juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::NoTextBox)
{
	positionSlider._slider.addListener(this);
	addAndMakeVisible(wc);
	addAndMakeVisible(&positionSlider._slider);
}
	
void WaveformAndPositionComponent::resized()
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
void WaveformAndPositionComponent::paint (juce::Graphics& g) {
	
}

void WaveformAndPositionComponent::sliderValueChanged (juce::Slider *slider) {
	if (slider == &positionSlider._slider){
		// this used to set the position of the read-only slider
	}
}
