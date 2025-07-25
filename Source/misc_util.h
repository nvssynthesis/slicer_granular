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
	
	// Public interface
	bool loadAudioFile(const juce::File& file);
	juce::AudioBuffer<float>& getSampleBuffer() { return sampleBuffer; }
	
	bool hasValidAudio() const { return sampleBuffer.getNumSamples() > 0; }

	const juce::String& getHash() const { return hash; }

	double getSampleRate() const { return sampleRate; }
	int getLength() const { return sampleBuffer.getNumSamples(); }
	int getNumChannels() const { return sampleBuffer.getNumChannels(); }
	
	juce::AudioFormatManager &getFormatManager() { return formatManager; }
private:
	juce::AudioFormatManager formatManager;
	juce::AudioBuffer<float> sampleBuffer;
	juce::String hash;
	
	double sampleRate {0.0};
	
	void computeHash();
	void clear();
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

inline juce::String hashAudioData(const std::vector<float>& audioData) {
	auto hash = juce::SHA256(audioData.data(), audioData.size() * sizeof(float));
	return hash.toHexString();
}

inline juce::String hashValueTree(const juce::ValueTree& settings)
{
	auto xmlElement = settings.createXml();
	if (!xmlElement) {
		std::cerr << "hashValueTree: !xmlElement\n";
		jassertfalse;
		return juce::String();
	}
	juce::String xmlString = xmlElement->toString();
	
	auto hash = juce::SHA256(xmlString.toUTF8(), xmlString.getNumBytesAsUTF8());
	return hash.toHexString();
}

inline bool isEmpty(juce::ValueTree const &vt){
	return (vt.getNumChildren() == 0 && vt.getNumProperties() == 0);
}
inline void clear(juce::ValueTree &vt){
	// Remove all children
	while (vt.getNumChildren() > 0) {
		vt.removeChild(0, nullptr);
	}

	// Remove all properties
	for (int i = vt.getNumProperties() - 1; i >= 0; --i) {
		vt.removeProperty(vt.getPropertyName(i), nullptr);
	}
}


inline juce::var valueTreeToVar(const juce::ValueTree& tree) {
	juce::DynamicObject::Ptr obj = new juce::DynamicObject();
	
	// Add type identifier
	obj->setProperty("type", tree.getType().toString());
	
	// Add all properties
	for (int i = 0; i < tree.getNumProperties(); ++i) {
		auto name = tree.getPropertyName(i);
		auto value = tree.getProperty(name);
		obj->setProperty(name, value);
	}
	
	// Add children
	if (tree.getNumChildren() > 0) {
		juce::Array<juce::var> children;
		for (auto child : tree) {
			children.add(valueTreeToVar(child));
		}
		obj->setProperty("children", children);
	}
	
	return juce::var(obj.get());
}

inline bool saveValueTreeToBinary(const juce::ValueTree& tree, const juce::File& file) {
	file.getParentDirectory().createDirectory();
	
	juce::FileOutputStream stream(file);
	if (stream.openedOk()) {
		tree.writeToStream(stream);
		return stream.getStatus().wasOk();
	}
	return false;
}
inline juce::ValueTree loadValueTreeFromBinary(const juce::File& file) {
	juce::FileInputStream stream(file);
	if (stream.openedOk()) {
		return juce::ValueTree::readFromStream(stream);
	}
	return juce::ValueTree();
}
inline bool saveValueTreeToJSON(const juce::ValueTree& tree, const juce::File& file) {
	file.getParentDirectory().createDirectory();
	
	juce::var jsonData = valueTreeToVar(tree);
	juce::String jsonString = juce::JSON::toString(jsonData);
	
	return file.replaceWithText(jsonString);
}


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
