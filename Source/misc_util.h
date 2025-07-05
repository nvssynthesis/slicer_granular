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
#include <fmt/format.h>
#include <string>

namespace nvs::util {

struct WeightedIdx
{
	int idx	{0};
	double weight{0.0}; // normalized probability (sums to 1 over all returned)
	
	WeightedIdx(int i, double w) : idx(i), weight(w) {}
	WeightedIdx() = default;
};
struct DistanceIdx	// effectively same class as weightedIdx but reminds you that we speak of distance, not weight
{
	int idx	{0};
	double distance{0.0}; // normalized probability (sums to 1 over all returned)
	
	DistanceIdx(int i, double d) : idx(i), distance(d) {}
	DistanceIdx() = default;
};

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
	juce::AudioBuffer<float> sampleBuffer;
	juce::int64 hash;
};

struct MeasuredData : public juce::ChangeBroadcaster
{
	std::vector<nvs::gran::GrainDescription> data0;
	std::vector<nvs::gran::GrainDescription> data1;
	std::atomic<bool> dataReady {false};
	std::atomic<int> activeBufferIdx {0};
};

template < typename C, C beginVal, C endVal>
class Iterator {
  typedef typename std::underlying_type<C>::type val_t;
  int val;
public:
  Iterator(const C & f) : val(static_cast<val_t>(f)) {}
  Iterator() : val(static_cast<val_t>(beginVal)) {}
  Iterator operator++() {
	++val;
	return *this;
  }
  C operator*() { return static_cast<C>(val); }
  Iterator begin() { return *this; } //default ctor is good
  Iterator end() {
	  static const Iterator endIter=++Iterator(endVal); // cache it
	  return endIter;
  }
  bool operator!=(const Iterator& i) { return val != i.val; }
};



struct TimedPrinter : public juce::Timer
{
	TimedPrinter(int intervalMs = 100)
		: criticalSection_(std::make_unique<juce::CriticalSection>())
	{
		startTimer(intervalMs);
	}
	
	~TimedPrinter() {
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

}	// nvs::util
