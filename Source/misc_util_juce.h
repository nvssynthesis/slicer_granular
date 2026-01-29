//
// Created by Nicholas Solem on 1/16/26.
//

#pragma once
#include <JuceHeader.h>
#include "Synthesis/GrainDescription.h"

namespace nvs::util {

struct MeasuredData : public juce::ChangeBroadcaster
{
    std::vector<nvs::gran::GrainDescription> data0;
    std::vector<nvs::gran::GrainDescription> data1;
    std::atomic<bool> dataReady {false};
    std::atomic<int> activeBufferIdx {0};
};

struct LoggingGuts {
    LoggingGuts()
    : logFile(File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getSiblingFile("log.txt"))
    , fileLogger(logFile, String(ProjectInfo::projectName) + " " + ProjectInfo::versionString + "logging")
    {
        Logger::setCurrentLogger (&fileLogger);
    }
    ~LoggingGuts();
    File logFile;
    FileLogger fileLogger;
    void logIfNaNOrInf(juce::AudioBuffer<float> buffer);
};

} // namespace nvs::util
