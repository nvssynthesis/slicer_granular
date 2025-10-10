/*
  ==============================================================================

    WaveformComponent.cpp
    Created: 9 Jan 2025 3:41:31pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "WaveformComponent.h"
#include <ranges>
#include "../SlicerGranularPluginProcessor.h"

WaveformComponent::WaveformComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample)
:	_proc(proc)
,	thumbnailCache(5), thumbnail(sourceSamplesPerThumbnailSample, proc.getAudioFormatManager(), thumbnailCache)
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
	l.applyTransform(juce::AffineTransform::scale(1.f, 0.5f));						// make line take up just 1 channel's worth of space (half the height)
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
	if (isMouseOver(true)){
		g.setColour(juce::Colours::whitesmoke.withMultipliedAlpha(0.15));
		g.fillAll();
		g.setColour(juce::Colours::whitesmoke.withMultipliedAlpha(0.76));
		
		g.setFont(juce::Font("Arial", 14.f, juce::Font::FontStyleFlags::plain));
		auto const f = juce::File(_proc.getSampleFilePath());
		auto const s = f.existsAsFile() ? f.getFileName() : "Right click or drag to load file";
		auto const textBounds = b.withTrimmedBottom(6).withTrimmedLeft(4);
		g.drawFittedText(s, textBounds, juce::Justification::bottomLeft, 1);
	}
	else {
//		g.setColour(juce::Colours::whitesmoke.withMultipliedAlpha(0.35));
	}
}


void WaveformComponent::thumbnailChanged()
{
	repaint();
}

void WaveformComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
	if (source == &thumbnail){
		thumbnailChanged();
	}
}
void WaveformComponent::highlightOnsets(std::vector<nvs::util::WeightedIdx> const &currentIndices) {
    std::vector<std::pair<double, double>> ranges;
    ranges.reserve(currentIndices.size());

    auto near_eq = [](double a, double b) {
        if (constexpr auto eps = std::numeric_limits<double>::lowest() * 10.0;
            std::abs(a - b) < eps){
            return true;
            }
        return false;
    };

    for (auto const &wi : currentIndices) {
        const auto currentIdx = wi.idx;
        if (currentIdx >= static_cast<int>(onsetMarkerList.size())) {
            return; // invalid
        }
        const auto nextIdx = (currentIdx + 1) % onsetMarkerList.size();
        const double startPos = onsetMarkerList[currentIdx].position;
        const double endPos = onsetMarkerList[nextIdx].position;

        jassert (!near_eq(startPos, endPos));

        ranges.push_back(std::make_pair(startPos, endPos));
    }
	highlightedRange = ranges;
	repaint();
}

void WaveformComponent::mouseUp(juce::MouseEvent const &e) {
	if (e.mods.isPopupMenu()) {
		juce::PopupMenu menu;
		
		menu.addItem(1, "Load Audio File...");
		menu.addItem(2, "Reveal current file directory");
		
		menu.showMenuAsync(juce::PopupMenu::Options{},
										[this](int result)
		  {
			if (result == 1) {
				auto chooser = std::make_shared<juce::FileChooser>("Select Audio File", juce::File{}, "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg");
				chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
									 [this, chooser](juce::FileChooser const &fc){
					auto file = fc.getResult();
					if (file.existsAsFile()){
						_proc.loadAudioFileAndUpdateState(file, true);
					}
				});
			}
			else if (result == 2) {
				// https://forum.juce.com/t/how-to-implement-reveal-in-finder/4373/2
				auto const path = _proc.getSampleFilePath();
				auto const file = juce::File(path);
				if (file.existsAsFile()){
					file.revealToUser();
				}
			}
			else {
				std::cout << "WaveformComponent::mouseUp: operation canceled\n";
			}
		  });
	}
}


bool WaveformComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
	for (const auto& file : files)
	{
		if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aiff") ||
			file.endsWithIgnoreCase(".aif")  || file.endsWithIgnoreCase(".mp3") ||
			file.endsWithIgnoreCase(".flac") || file.endsWithIgnoreCase(".ogg"))
			return true;
	}
	return false;
}
void WaveformComponent::fileDragEnter (const juce::StringArray&, int, int)
{
	isDragOver = true;
	repaint();
}

void WaveformComponent::fileDragExit (const juce::StringArray&)
{
	isDragOver = false;
	repaint();
}

void WaveformComponent::filesDropped (const juce::StringArray& files, int, int)
{
	isDragOver = false;
	if (files.size() > 0) {
		_proc.loadAudioFileAndUpdateState(files[0], true);
	}
	repaint();
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

WaveformAndPositionComponent::WaveformAndPositionComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample)
:	wc(proc, sourceSamplesPerThumbnailSample)
,	positionSlider(proc.getAPVTS(), nvs::param::ParameterRegistry::getParameterByID("position"), juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::NoTextBox)
{
	addAndMakeVisible(wc);
	addAndMakeVisible(&positionSlider._slider);
}
	
void WaveformAndPositionComponent::hideSlider() {
	positionSlider._slider.setVisible(false);
	resized();
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

		auto const wcBounds = juce::Rectangle<int>(waveformX, waveformY, waveformWidth, waveformHeight);
	    wc.setBounds(wcBounds);
		return wcBounds;	// return rectangle as that's all we need from this scope
	}();

	if (sliderVisible)
	{
		auto const sliderHeight = reservedHeight - wcRect.getHeight();
        constexpr int widthIncrease = 14;
		auto const sliderWidth = wcRect.getWidth() + widthIncrease;
		auto const sliderX = wcRect.getX() - (widthIncrease*0.5);
		auto const sliderY = wcRect.getBottom();
		auto const sliderRect = juce::Rectangle<int>(sliderX, sliderY, sliderWidth, sliderHeight);
		positionSlider._slider.setBounds(sliderRect);
	}
}

