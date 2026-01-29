//
// Created by Nicholas Solem on 11/7/25.
//

#pragma once
#include <JuceHeader.h>

namespace nvs::axiom {
#ifndef STRAXIOMIZE
#define STRAXIOMIZE(x) inline constexpr const char* x {#x}
#endif

STRAXIOMIZE(PLUGIN_STATE);
STRAXIOMIZE(PARAM);
STRAXIOMIZE(FileInfo);

STRAXIOMIZE(Version);
STRAXIOMIZE(sampleFilePath);
STRAXIOMIZE(sampleRate);
STRAXIOMIZE(audioHash);

inline const juce::String AudioFilePathAbsolute = "AudioFilePath (absolute)";

STRAXIOMIZE(position);

#undef STRAXIOMIZE
}