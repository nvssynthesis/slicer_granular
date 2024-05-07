/*
  ==============================================================================

    JuceGranularSynthSound.cpp
    Created: 7 May 2024 11:04:14am
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "JuceGranularSynthSound.h"

bool GranularSound::appliesToNote (int midiNoteNumber) {
	return true;
}
bool GranularSound::appliesToChannel (int midiChannel) {
	return true;
}
