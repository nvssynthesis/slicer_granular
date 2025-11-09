//
// Created by Nicholas Solem on 11/7/25.
//

#pragma once
#include <JuceHeader.h>

namespace nvs::axiom {
#define STRAXIOMIZE(x) inline constexpr const char* x {#x}

STRAXIOMIZE(PLUGIN_STATE);
STRAXIOMIZE(PARAM);

STRAXIOMIZE(Metadata);
STRAXIOMIZE(Version);
STRAXIOMIZE(FileInfo);
STRAXIOMIZE(CreationTime);
STRAXIOMIZE(sampleFilePath);
STRAXIOMIZE(sampleRate);
STRAXIOMIZE(audioHash);
STRAXIOMIZE(settingsHash);
STRAXIOMIZE(AnalysisSettings);
STRAXIOMIZE(Settings);
STRAXIOMIZE(analysisFile);

STRAXIOMIZE(TimbreAnalysis);
STRAXIOMIZE(NormalizedOnsets);
STRAXIOMIZE(TimbreMeasurements);

STRAXIOMIZE(x_axis);
STRAXIOMIZE(y_axis);
STRAXIOMIZE(z_axis);
STRAXIOMIZE(w_axis);
STRAXIOMIZE(u_axis);
STRAXIOMIZE(v_axis);
STRAXIOMIZE(nav_tendency_x);
STRAXIOMIZE(nav_tendency_y);
STRAXIOMIZE(histogram_equalization);

STRAXIOMIZE(statistic);
STRAXIOMIZE(mean);
STRAXIOMIZE(median);
STRAXIOMIZE(variance);
STRAXIOMIZE(skewness);
STRAXIOMIZE(kurtosis);

STRAXIOMIZE(Frame);
STRAXIOMIZE(BFCCs);
STRAXIOMIZE(Periodicity);
STRAXIOMIZE(Loudness);
STRAXIOMIZE(F0);

STRAXIOMIZE(position);

inline const juce::String AudioFilePathAbsolute = "AudioFilePath (absolute)";

STRAXIOMIZE(saveAnalysis);
STRAXIOMIZE(onsetsAvailable);

STRAXIOMIZE(tsn_granular);
STRAXIOMIZE(Analyses);

#undef STRAXIOMIZE
}