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
	
int WaveformComponent::getNumMarkers(MarkerType markerType){
	return markerListMap.at(markerType).getNumMarkers();
}
void WaveformComponent::addMarker(double pos, MarkerType markerType){
	juce::String name(pos);
	markerListMap.at(markerType).setMarker(name, juce::RelativeCoordinate(pos));
}
void WaveformComponent::removeMarkers(MarkerType markerType){
	juce::MarkerList markerList = markerListMap.at(markerType);
	auto numMarkers = markerList.getNumMarkers();
	for (auto i = numMarkers - 1; i >= 0 ; --i){
		markerList.removeMarker(i);
	}
}

void WaveformComponent::drawMarker(juce::Graphics& g, double pos, MarkerType markerType){
	auto const line = [&]
	{
		float const xPos = getWidth() * pos;
		float y0 = getLocalBounds().getY();
		float y1 = getLocalBounds().getBottom();
		auto l = juce::Line<float>(juce::Point<float>{xPos, y0}, juce::Point<float>{xPos, y1});
		if (markerType == MarkerType::Onset){
			g.setColour(juce::Colours::blue);

		}
		else if (markerType == MarkerType::CurrentPosition){
			g.setColour(juce::Colours::lightgreen);
			auto const shortenBy = l.getLength() * 0.15;
			l = l.withShortenedStart(shortenBy);
			l = l.withShortenedEnd(shortenBy);
		}
		return l;
	}();
	
	g.drawLine(line, 1.f);
}
void WaveformComponent::drawMarkers(juce::Graphics& g, MarkerType markerType){
	auto markerList = markerListMap.at(markerType);
	for (auto i = 0; i < markerList.getNumMarkers(); ++i){
		double pos = markerList.getMarkerPosition(*markerList.getMarker(i), this);
		drawMarker(g, pos, markerType);
	}
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
