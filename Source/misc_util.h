/*
  ==============================================================================

    misc_util.h
    Created: 20 Apr 2024 3:24:24pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Synthesis/GrainDescription.h"

namespace nvs::util {
struct LoggingGuts {
	LoggingGuts();	// constructor wants to be defined in the corresponding plugin processor
	~LoggingGuts();
	juce::File logFile;
	juce::FileLogger fileLogger;
	void logIfNaNOrInf(juce::AudioBuffer<float> buffer);
};

struct SampleManagementGuts : public juce::ChangeBroadcaster
{
	SampleManagementGuts();
	~SampleManagementGuts();
	juce::AudioFormatManager formatManager;
	const juce::String audioFilePathValueTreeStateIdentifier {"sampleFilePath"};
	juce::String sampleFilePath;
	juce::AudioBuffer<float> sampleBuffer;
	double lastFileSampleRate { 0.0 };
	float normalizationValue { 1.f };	// a MULTIPLIER for the overall output, based on the inverse of the absolute max value for the current sample
};

struct MeasuredData : public juce::ChangeBroadcaster
{
	std::vector<nvs::gran::GrainDescription> data0;
	std::vector<nvs::gran::GrainDescription> data1;
	std::atomic<bool> dataReady {false};
	std::atomic<int> activeBufferIdx {0};
};
}	// nvs::util
