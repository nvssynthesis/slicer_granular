/*
  ==============================================================================

    WaveformComponent.cpp
    Created: 9 Jan 2025 3:41:31pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "WaveformComponent.h"
#include <ranges>

#ifdef TSN
#include "../../../Source/Synthesis/TSNGranularSynthesizer.h"
#endif

WaveformComponent::WaveformComponent(int sourceSamplesPerThumbnailSample, juce::AudioFormatManager &formatManagerToUse)
:	thumbnailCache(5), thumbnail(sourceSamplesPerThumbnailSample, formatManagerToUse, thumbnailCache)
{
	thumbnail.addChangeListener(this);	// thumbnail is a ChangeBroadcaster
}
	
size_t WaveformComponent::getNumMarkers(MarkerType markerType) {
	const auto& markerListVariant = markerListMap.at(markerType);
	return std::visit([](auto* markerList) { return markerList->size(); }, markerListVariant);
}
std::vector<double> WaveformComponent::getNormalizedOnsets() const {
	auto view = onsetMarkerList | std::views::transform([](OnsetMarker const &m){ return m.position; }) ;
	std::vector<double> onsets(std::ranges::begin(view), std::ranges::end(view));
	return onsets;
}

void WaveformComponent::addMarker(double onsetPosition) {
	assert (onsetPosition >= 0.0);
	assert (onsetPosition <= 1.0);
	auto it = std::lower_bound(onsetMarkerList.begin(), onsetMarkerList.end(), onsetPosition,
							   [](const OnsetMarker& marker, double position) {
								   return marker.position < position;
							   });

	onsetMarkerList.insert(it, OnsetMarker{onsetPosition});
}
void WaveformComponent::addMarker(nvs::gran::GrainDescription const &gd){
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
namespace {
void processLine(juce::Graphics& g, juce::Line<float> &, WaveformComponent::OnsetMarker const &){
	g.setColour(juce::Colour(juce::Colours::blue).withMultipliedAlpha(0.75f));
}
void processLine(juce::Graphics& g, juce::Line<float> &l, WaveformComponent::PositionMarker const &marker){
	auto const regionHeight = l.getLength();
	[[maybe_unused]] int const g_id = marker.grain_id;
	auto const r = marker.sample_playback_rate;
	auto const p = marker.pan;
	auto const w = marker.window;
	auto const busy = marker.busy;
	auto const first_playthrough = marker.first_playthrough;
	if (!busy){
		assert (w == 0.f);
	}
	juce::Colour colour = busy ? juce::Colour(juce::Colours::lightgreen).withMultipliedBrightness(1.1f) : juce::Colour(juce::Colours::grey).withMultipliedLightness(0.9f);

	if (!first_playthrough){
		colour = colour.withRotatedHue(log2(r) / 20.f);									// pitch affects hue
		g.setColour(colour);
		g.setOpacity(sqrt(w));															// envelope (window) affects opacity
	}
	else {
		colour = colour.withAlpha(0.f);
		g.setColour(colour);
	}
	l.applyTransform(juce::AffineTransform::translation(0.0f, p * (regionHeight)));	// panning affects y position
	l.applyTransform(juce::AffineTransform::scale(1.f, 0.5f));						// make line take up just 1 channel's worth of space (half the height)}
}
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
		assert (y1 > y0);
		auto l = juce::Line<float>(juce::Point<float>{xPos, y0}, juce::Point<float>{xPos, y1});
		std::visit([&](const auto &marker) {
			processLine(g, l, marker);
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
	
	
	auto const b = getBounds();
	if (highlightedRange.has_value()){
		for (auto const &r : *highlightedRange){
			float const w = b.getWidth();
			float const low = r.first;
			float const high = r.second;
			g.setColour(juce::Colour(juce::Colours::whitesmoke).withAlpha(0.5f));
			if (low < high){
				float const p0 = b.getX() + low * w;
				float const newWidth = (high - low) * w ;
				auto const selectedRect = b.withX(p0).withWidth(newWidth);
				g.fillRect(selectedRect);
			}
			else {	// low > high
				jassert (high != low);	// there should ave been logic in place to prevent overlapping onsets
				float const newWidth = b.getX() + high * w;
				auto const lowerRect = b.withWidth(newWidth);
				g.fillRect(lowerRect);
				float const newX = low * w;
				auto const upperRect = b.withX(newX);
				g.fillRect(upperRect);
			}
		}
	}
}

void WaveformComponent::highlight(std::vector<std::pair<double, double>> rangeToHighlight)
{
	highlightedRange = rangeToHighlight;
	repaint();
}

void WaveformComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
	if (source == &thumbnail){
		thumbnailChanged();
	}
#ifdef TSN
	else if (auto *tsn_synth = dynamic_cast<TSNGranularSynthesizer*>(source)){
		std::vector<TSNGranularSynthesizer::WeightedIdx> currentIndices = tsn_synth->getCurrentIndices();
		std::vector<std::pair<double, double>> ranges;
		ranges.reserve(currentIndices.size());
		
		auto near_eq = [](double a, double b){
			double eps = std::numeric_limits<double>::lowest() * 10.0;
			if (std::abs(a - b) < eps){
				return true;
			}
			return false;
		};
		
		for (auto const &wi : currentIndices){
			auto currentIdx = wi.idx;
			jassert ((currentIdx == 0) or (currentIdx < (int)onsetMarkerList.size()));
			auto nextIdx = (currentIdx + 1) % onsetMarkerList.size();
			double startPos = onsetMarkerList[currentIdx].position;
			double endPos = onsetMarkerList[nextIdx].position;
			
			
			jassert (!near_eq(startPos, endPos));
			
			ranges.push_back(std::make_pair(startPos, endPos));
		}
		
		highlight(ranges);
	}
#endif
}
	
void WaveformComponent::setThumbnailSource (const juce::AudioBuffer<float> *newSource, double sampleRate, juce::int64 hashCode){
	thumbnail.setSource(newSource, sampleRate, hashCode);
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
,	positionSlider(apvts, nvs::param::ParameterRegistry::getParameterByID("position"), juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::NoTextBox)
{
	addAndMakeVisible(wc);
	addAndMakeVisible(&positionSlider._slider);
}
	
void WaveformAndPositionComponent::hideSlider() {
	positionSlider._slider.setVisible(false);
	repaint();
}

void WaveformAndPositionComponent::resized()
{
	auto const localBounds = getLocalBounds();
	
	auto const totalHeight = localBounds.getHeight();
	auto const reservedHeight = totalHeight * 0.9;

	bool const sliderVisible = positionSlider._slider.isVisible();
	
	juce::Rectangle<int> const wcRect = [&]{	// limit scope via instantly-called lambda
		auto const heightDiff = totalHeight - reservedHeight;
		auto const waveformY = localBounds.getY() + (heightDiff * 0.5);
		auto const waveformHeight = sliderVisible ? reservedHeight * 0.8f : reservedHeight;
		
		auto const waveformWidth = localBounds.getWidth() * 1.0;
		auto const waveformWidthDiff = localBounds.getWidth() - waveformWidth;
		auto const waveformX = localBounds.getX() + (waveformWidthDiff * 0.5);

		auto const wcRect = juce::Rectangle<int>(waveformX, waveformY, waveformWidth, waveformHeight);
		wc.setBounds(wcRect);
		return wcRect;	// return rectangle as that's all we need from this scope
	}();
	
	
	
	if (sliderVisible)
	{
		auto const sliderHeight = reservedHeight - wcRect.getHeight();
		int const widthIncrease = 14;
		auto const sliderWidth = wcRect.getWidth() + widthIncrease;
		auto const sliderX = wcRect.getX() - (widthIncrease*0.5);
		auto const sliderY = wcRect.getBottom();
		auto const sliderRect = juce::Rectangle<int>(sliderX, sliderY, sliderWidth, sliderHeight);
		positionSlider._slider.setBounds(sliderRect);
	}
}

void WaveformAndPositionComponent::paint (juce::Graphics& g) {}
