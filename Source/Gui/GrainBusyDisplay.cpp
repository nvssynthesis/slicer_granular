/*
  ==============================================================================

    GrainBusyDisplay.cpp
    Created: 17 Feb 2025 3:05:59pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GrainBusyDisplay.h"

void GrainBusyDisplay::paint(Graphics &g){
	const auto clearColour {Colour(
	    static_cast<juce::uint8>(0),
	    static_cast<juce::uint8>(0),
	    static_cast<juce::uint8>(0),
	    static_cast<juce::uint8>(0))
	};
	g.setColour(clearColour);
	g.fillAll();
	
	g.setColour(juce::Colours::black);
	for (size_t i = 0; i < _statuses.size(); ++i){
	    const auto voiceAndGrain = getVoiceAndGrain(i);
		const int status = _statuses[getIndex(voiceAndGrain.voice, voiceAndGrain.grain)];
	    const int x = static_cast<int>(std::round(static_cast<float>(voiceAndGrain.grain) * _sizePerGrain));
		const int y = static_cast<int>(std::round(static_cast<float>(voiceAndGrain.voice) * _sizePerGrain));
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
