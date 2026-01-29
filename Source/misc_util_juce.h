//
// Created by Nicholas Solem on 1/16/26.
//

#pragma once
#include <JuceHeader.h>
#include "Synthesis/GrainDescription.h"
#include "fmt/core.h"

namespace nvs::util {

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

juce::String computeHash(const juce::AudioBuffer<float> &bufferToHash);

struct SampleManagementGuts : public juce::ChangeBroadcaster
{
    SampleManagementGuts();
    ~SampleManagementGuts() override;
    using AudioBuffer = juce::AudioBuffer<float>;

    bool loadAudioFile(const juce::File& file);
    AudioBuffer& getSampleBuffer() { return sampleBuffer; }

    bool hasValidAudio() const { return sampleBuffer.getNumSamples() > 0; }

    const juce::String& getWaveformHash() const { return waveformHash; }

    double getSampleRate() const { return sampleRate; }
    int getLength() const { return sampleBuffer.getNumSamples(); }
    int getNumChannels() const { return sampleBuffer.getNumChannels(); }

    juce::AudioFormatManager &getFormatManager() { return formatManager; }
private:
    juce::AudioFormatManager formatManager;
    AudioBuffer sampleBuffer;
    juce::String waveformHash;
    double sampleRate {0.0};

    void clear();
};

struct MeasuredData : public juce::ChangeBroadcaster
{
    std::vector<nvs::gran::GrainDescription> data0;
    std::vector<nvs::gran::GrainDescription> data1;
    std::atomic<bool> dataReady {false};
    std::atomic<int> activeBufferIdx {0};
};

struct TimedPrinter final : public juce::Timer
{
	TimedPrinter(int intervalMs = 100)
		: criticalSection_(std::make_unique<juce::CriticalSection>())
	{
		startTimer(intervalMs);
	}

	~TimedPrinter() override {
		stopTimer();
	}

	// Fast print function - just stores the formatted string
	template<typename... Args>
	void print(fmt::format_string<Args...> format_str, Args&&... args) {
		std::string formatted = fmt::format(format_str, std::forward<Args>(args)...);

		juce::ScopedLock lock(*criticalSection_);
		pending_message_ = std::move(formatted);
	}

	// Raw string version
	void printRaw(const std::string& message) {
		juce::ScopedLock lock(*criticalSection_);
		pending_message_ = message;
	}

	void timerCallback() override {
		std::string message_to_print;

		{
			juce::ScopedLock lock(*criticalSection_);
			if (!pending_message_.empty()) {
				message_to_print = std::move(pending_message_);
				pending_message_.clear();
			}
		}

		if (!message_to_print.empty()) {
			std::cout << message_to_print << std::endl;
		}
	}

	// Change print interval
	void setInterval(int intervalMs) {
		stopTimer();
		startTimer(intervalMs);
	}

	// Force immediate print (bypass timer)
	void flush() {
		std::string message_to_print;

		{
			juce::ScopedLock lock(*criticalSection_);
			if (!pending_message_.empty()) {
				message_to_print = std::move(pending_message_);
				pending_message_.clear();
			}
		}

		if (!message_to_print.empty()) {
			std::cout << message_to_print << std::endl;
		}
	}

private:
	std::unique_ptr<juce::CriticalSection> criticalSection_;
	std::string pending_message_;
};

// Usage example:
/*
TimedPrinter printer(50); // Print every 50ms

// In your audio processing loop:
for (int sample = 0; sample < numSamples; ++sample) {
	float value = processAudio(sample);

	// This is now thread-safe
	printer.print("Sample {}: value = {:.3f}", sample, value);
}
*/

} // namespace nvs::util
