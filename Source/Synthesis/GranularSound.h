/*
  ==============================================================================

    JuceGranularSynthSound.h
    Created: 7 May 2024 11:04:14am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>


class GranularSound	:	public juce::SynthesiserSound
{
public:
	bool appliesToNote (int midiNoteNumber) override;
	bool appliesToChannel (int midiChannel) override;
};
