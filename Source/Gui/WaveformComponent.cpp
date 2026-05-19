/*
  ==============================================================================

    WaveformComponent.cpp
    Created: 9 Jan 2025 3:41:31pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "WaveformComponent.h"
#include "../SlicerGranularPluginProcessor.h"
#include <ranges>

WaveformComponent::WaveformComponent(SlicerGranularAudioProcessor &proc, const int sourceSamplesPerThumbnailSample)
:	_proc(proc)
,	thumbnailCache(5), thumbnail(sourceSamplesPerThumbnailSample, proc.getAudioFormatManager(), thumbnailCache)
{
	thumbnail.addChangeListener(this);	// thumbnail is a ChangeBroadcaster
}

void WaveformComponent::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == &thumbnail){
        thumbnailChanged();
    }
}

size_t WaveformComponent::getNumMarkers(const MarkerType markerType) const {
	const auto& markerListVariant = markerListMap.at(markerType);
	return std::visit([](auto* markerList) { return markerList->size(); }, markerListVariant);
}
std::vector<double> WaveformComponent::getNormalizedOnsets() const {
	auto view = onsetMarkerList | std::views::transform([](OnsetMarker const &m){ return m.position; }) ;
	std::vector onsets(std::ranges::begin(view), std::ranges::end(view));
	return onsets;
}

void WaveformComponent::addMarker(const double onsetPosition) {
	assert (onsetPosition >= 0.0);
	assert (onsetPosition <= 1.0);
	const auto it = std::lower_bound(onsetMarkerList.begin(), onsetMarkerList.end(), onsetPosition,
							   [](const OnsetMarker& marker, const double position) {
								   return marker.position < position;
							   });

	onsetMarkerList.insert(it, OnsetMarker{onsetPosition});
}
void WaveformComponent::addMarker(nvs::gran::GrainDescription const &gd){
	const auto it = std::lower_bound(currentPositionMarkerList.begin(), currentPositionMarkerList.end(), gd,
							   [](const PositionMarker& marker, const nvs::gran::GrainDescription &gd_) {
									return marker.position < gd_.position;
							   });
	currentPositionMarkerList.insert(it, PositionMarker::fromGrainDescription(gd));
}
void WaveformComponent::removeMarkers(const MarkerType markerType) {
	auto& markerListVariant = markerListMap.at(markerType);

	std::visit([](auto* markerList) {
		markerList->clear();
		assert(markerList->empty());
	}, markerListVariant);
}
void WaveformComponent::drawMarkers(Graphics& g, const MarkerType markerType){
	auto const &markerListVariant = markerListMap.at(markerType);
	std::visit([&](auto const& markerList) {
		for (auto const& marker : *markerList) {
			drawMarker(g, marker);
		}
	}, markerListVariant);
}
namespace {
void processLine(Graphics& g, Line<float> &, WaveformComponent::OnsetMarker const &){
	g.setColour(Colour(Colours::blue).withMultipliedAlpha(0.1f));
#pragma message("alpha should depend on neighboring onset density")
}
void processLine(Graphics& g, Line<float> &l, WaveformComponent::PositionMarker const &marker){
	auto const regionHeight = l.getLength();
	[[maybe_unused]] int const g_id = marker.grain_id;
	auto const r = marker.sample_playback_rate;
	auto const p = marker.pan;
	auto const w = marker.window;
	auto const busy = marker.busy;
	if (!busy){
		assert (w == 0.f);
	}
	Colour colour = busy ? Colour(Colours::lightgreen).withMultipliedBrightness(1.1f) : Colour(Colours::grey).withMultipliedLightness(0.9f);

	colour = colour.withRotatedHue(log2(static_cast<float>(r)) / 20.f);	// pitch affects hue
	g.setColour(colour);
	g.setOpacity(nvs::memoryless::clamp(sqrt(w), 0.f, 1.f));				// envelope (window) affects opacity

	l.applyTransform(AffineTransform::translation(0.0f, p * regionHeight));	// panning affects y position
	l.applyTransform(AffineTransform::scale(1.f, 0.5f));						// make line take up just 1 channel's worth of space (half the height)
}
}
void WaveformComponent::drawMarker(Graphics& g, MarkerVariant marker)
{
	auto const line = [&]
	{
		const double position = std::visit([](const auto& m) {
			return m.position; // position is a common member to all alternatives
		}, marker);
		const float xPos = getWidth() * position;
		const auto y0 = static_cast<float>(waveformBounds.getY());
		const auto y1 = static_cast<float>(waveformBounds.getBottom());
		assert (y1 > y0);
		auto l = Line(Point{xPos, y0}, Point{xPos, y1});
		std::visit([&](const auto &m) {
			processLine(g, l, m);
		}, marker);
		return l;
	}();
	
	g.drawLine(line, 1.f);
}

void WaveformComponent::resized() {
	waveformBounds = getLocalBounds();
}

void WaveformComponent::paint(Graphics& g)
{
	g.setColour (Colours::darkgrey);
	auto const &b = waveformBounds;
	g.drawRect(b);
	
	if (thumbnail.getNumChannels() == 0) {
		paintContentsIfNoFileLoaded (g);
	}
	else {
		paintContentsIfFileLoaded (g);
	}
	drawMarkers(g, MarkerType::Onset);
	drawMarkers(g, MarkerType::CurrentPosition);
	

	if (highlightedRange.has_value()){
		for (const auto &[low, high] : *highlightedRange){
			float const w = b.getWidth();
			g.setColour(Colour(Colours::whitesmoke).withAlpha(0.5f));
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
		g.setColour(Colours::whitesmoke.withMultipliedAlpha(0.15));
		g.fillRect(waveformBounds);
		g.setColour(Colours::whitesmoke.withMultipliedAlpha(0.76));
		
		g.setFont(FontOptions("Arial", 14.f, Font::FontStyleFlags::plain));
		auto const f = File(_proc.getSampleFilePath());
		auto const s = f.existsAsFile() ? f.getFileName() : "Right click or drag to load file";
		auto const textBounds = b.withTrimmedBottom(6).withTrimmedLeft(4);
		g.drawFittedText(s, textBounds, Justification::bottomLeft, 1);
	}
	else {
//		g.setColour(Colours::whitesmoke.withMultipliedAlpha(0.35));
	}
}


void WaveformComponent::thumbnailChanged()
{
	repaint();
}

void WaveformComponent::highlightOnsets(std::vector<nvs::timbrespace::WeightedIdx> const &currentIndices) {
    std::vector<std::pair<double, double>> ranges;
    ranges.reserve(currentIndices.size());

    auto near_eq = [](const double a, const double b) {
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

        ranges.emplace_back(std::make_pair(startPos, endPos));
    }
	highlightedRange = ranges;
	repaint();
}

void WaveformComponent::mouseUp(MouseEvent const &e) {
	if (e.mods.isPopupMenu()) {
		PopupMenu menu;
		
		menu.addItem(1, "Load Audio File...");
		menu.addItem(2, "Reveal current file directory");
		
		menu.showMenuAsync(PopupMenu::Options{},
										[this](const int result)
		  {
			if (result == 1) {
				auto chooser = std::make_shared<FileChooser>("Select Audio File", File{}, "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg");
				chooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
								[this, chooser](FileChooser const &fc)
								{
                                    if (const auto file = fc.getResult(); file.existsAsFile())
                                    {
						                _proc.loadAudioFileAndUpdateState(file, true);
					                }
				                });
			}
			else if (result == 2) {
				// https://forum.juce.com/t/how-to-implement-reveal-in-finder/4373/2
				auto const path = _proc.getSampleFilePath();
				auto const file = File(path);
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


bool WaveformComponent::isInterestedInFileDrag (const StringArray& files)
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
void WaveformComponent::fileDragEnter (const StringArray&, int, int)
{
	isDragOver = true;
	repaint();
}

void WaveformComponent::fileDragExit (const StringArray&)
{
	isDragOver = false;
	repaint();
}

void WaveformComponent::filesDropped (const StringArray& files, int, int)
{
	isDragOver = false;
	if (files.size() > 0) {
		_proc.loadAudioFileAndUpdateState(files[0], true);
	}
	repaint();
}
void WaveformComponent::setThumbnailSource (const AudioBuffer<float> *newSource, const double sampleRate, const int64 hashCode){
	thumbnail.setSource(newSource, sampleRate, hashCode);
}
int64 WaveformComponent::getHashCode() const {
    return thumbnail.getHashCode();
}

void WaveformComponent::paintContentsIfNoFileLoaded (Graphics& g)
{
	g.setColour (Colours::darkgrey);
	g.drawFittedText ("No File Loaded", waveformBounds, Justification::centred, 1);
}
void WaveformComponent::paintContentsIfFileLoaded (Graphics& g)
{
	g.setColour (Colours::black);
	thumbnail.drawChannels (g,
							waveformBounds.withTrimmedRight(1),
							0.0,                                    // start time
							thumbnail.getTotalLength(),             // end time
							1.0f);                   // vertical zoom

    // TODO: Draw envelope using (probably) juce::Path
#pragma message("draw envelope")
}

//================================================================================================================================================


WaveformAndPositionComponent::WaveformAndPositionComponent(SlicerGranularAudioProcessor &proc, int sourceSamplesPerThumbnailSample)
:	WaveformComponent(proc, sourceSamplesPerThumbnailSample)
,	positionSlider(proc.getAPVTS(), nvs::param::ParameterRegistry::getParameterByID(nvs::axiom::position), Slider::SliderStyle::LinearHorizontal, Slider::NoTextBox)
{
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
	
	{
		auto const heightDiff = totalHeight - reservedHeight;
		auto const waveformY = localBounds.getY() + 0.5*heightDiff;
		auto const waveformHeight = sliderVisible ? reservedHeight * 0.8f : reservedHeight;
		
		auto const waveformWidth = localBounds.getWidth() * 1.0;
		auto const waveformWidthDiff = localBounds.getWidth() - waveformWidth;
		auto const waveformX = localBounds.getX() + 0.5*waveformWidthDiff;

		waveformBounds = Rectangle(waveformX, waveformY, waveformWidth, waveformHeight).toNearestInt();
	}

	if (sliderVisible)
	{
		auto const sliderHeight = static_cast<int>(reservedHeight) - waveformBounds.getHeight();
        constexpr int widthIncrease = 14;
		auto const sliderWidth = waveformBounds.getWidth() + widthIncrease;
		auto const sliderX = waveformBounds.getX() - widthIncrease/2;
		auto const sliderY = waveformBounds.getBottom();
		auto const sliderRect = Rectangle(sliderX, sliderY, sliderWidth, sliderHeight);
		positionSlider._slider.setBounds(sliderRect);
	}
}

