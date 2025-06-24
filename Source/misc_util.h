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

using timbre2DPoint = juce::Point<float>;
using timbre3DPoint = std::array<float, 3>;

struct Timbre5DPoint {

	timbre2DPoint _p2D;			// used to locate the point in x,y plane
	timbre3DPoint _p3D;	// used to describe the colour (hsv)

	bool operator==(Timbre5DPoint const &other) const;
	timbre2DPoint get2D() const { return _p2D; }
	timbre3DPoint get3D() const { return _p3D; }

	// to easily trade hsv for rbg
	std::array<juce::uint8, 3> toUnsigned() const;
};

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

class TimbreSpaceHolder {
public:
	void add5DPoint(timbre2DPoint p2D, std::array<float, 3> p3D);
	void clear();
	
	juce::Array<nvs::util::Timbre5DPoint> &getTimbreSpace() {
		return timbres5D;
	}
	std::vector<WeightedIdx> getCurrentPointIndices() const {
		return currentPointIndices;
	}
	void setProbabilisticPointFromTarget(const Timbre5DPoint& target, int K_neighbors, double sharpness, float higher3Dweight=0.f);

private:
	juce::Array<nvs::util::Timbre5DPoint> timbres5D;
	
	// safe private setter
	void setCurrentPointIndices(std::vector<WeightedIdx> &&newWeightedIndices);
	
	std::vector<WeightedIdx> currentPointIndices {{},{},{},{}};
};

constexpr bool inRange0_1(float x){
	return ( (x >= 0.f) && (x <= 1.f) );
}
constexpr bool inRangeM1_1(float x){
	return ( (x >= -1.f) && (x <= 1.f) );
}

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

}	// nvs::util
