/*
  ==============================================================================

    GrainBusyDisplay.h
    Created: 17 Feb 2025 3:05:59pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../Synthesis/VoicesXGrains.h"


class GrainBusyDisplay	:	public juce::Component
	// needs to paint squares
	// lit up if the corresponding grain is active,
	// otherwise just the outline
	// from outside, the dimensions this is given should be S * N_GRAINS, S * N_VOICES
{
public:	
	void paint(juce::Graphics &g) override;
	void resized() override;
	
	void setStatus(int grain, int voice, bool status){
		_statuses[getIndex(voice, grain)] = status;
	}
	void setSizePerGrain(float s) {
		_sizePerGrain = s;
	}
	
	float getSizePerGrain() const {
		return _sizePerGrain;
	}

private:
	float _sizePerGrain  {0};
	
	size_t getIndex(size_t voice, size_t grain) {
		size_t idx = voice * N_GRAINS + grain;
		return idx;
	}
	struct VoiceAndGrain {
		size_t voice;
		size_t grain;
	};
	VoiceAndGrain getVoiceAndGrain(size_t idx){
		return VoiceAndGrain {
			.voice = idx / N_GRAINS,
			.grain = idx % N_GRAINS
		};
	}
	
	std::array<bool, N_GRAINS * N_VOICES> _statuses {};
};
