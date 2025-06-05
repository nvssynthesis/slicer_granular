/*
  ==============================================================================

    JuceGranularSynthSound.cpp
    Created: 7 May 2024 11:04:14am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularSound.h"

bool GranularSound::appliesToNote (int midiNoteNumber) {
	(void)midiNoteNumber;
	return true;
}
bool GranularSound::appliesToChannel (int midiChannel) {
	(void)midiChannel;
	return true;
}
