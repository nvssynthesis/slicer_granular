/*
  ==============================================================================

    GrainBusyDisplay.cpp
    Created: 17 Feb 2025 3:05:59pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GrainBusyDisplay.h"

void GrainBusyDisplay::paint(juce::Graphics &g){
	juce::Colour clearColour {juce::Colour(juce::uint8(0), juce::uint8(0), juce::uint8(0), juce::uint8(0))};
	g.setColour(clearColour);
	g.fillAll();
	
	g.setColour(juce::Colours::black);
	for (size_t i = 0; i < _statuses.size(); ++i){
		auto const voiceAndGrain = getVoiceAndGrain(i);
		int status = _statuses[getIndex(voiceAndGrain.voice, voiceAndGrain.grain)];
		int const x = voiceAndGrain.grain * _sizePerGrain;
		int const y = voiceAndGrain.voice * _sizePerGrain;
		if (status){
			g.fillRect(x, y, (int)_sizePerGrain, (int)_sizePerGrain);
		}
		else {
			g.drawRect(x, y, (int)_sizePerGrain, (int)_sizePerGrain);
		}
	}
}

void GrainBusyDisplay::resized(){
	
}
