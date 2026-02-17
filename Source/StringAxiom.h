//
// Created by Nicholas Solem on 11/7/25.
//

#pragma once
#include <JuceHeader.h>
#include <StringAxiom.h>

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

STRAXIOMIZE(position);

STRAXIOMIZE(frequency_randomization_mode);
STRAXIOMIZE(Continuous);
STRAXIOMIZE(Octaves);

namespace tsn {
#pragma message("move to StringAxiom of tsn-analyzer after pull")
STRAXIOMIZE(pitchify);
}

#undef STRAXIOMIZE
}
