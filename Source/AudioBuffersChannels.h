/*
  ==============================================================================

    AudioBuffersChannels.h
    Created: 27 Oct 2023 3:28:12pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once

class AudioBuffersChannels
{
public:
	using stereoVector_t = std::array<std::vector<float>, 2>;
	AudioBuffersChannels(){
		activeSpan = std::span<float>(stereoBuffers[activeBufferIdx][0]);
	}
	std::span<float> const &getActiveSpanRef(){
		return activeSpan;
	}
	[[nodiscard]]
	std::array<float *const, 2> prepareForWrite(size_t length, size_t numChannels){
		auto inactiveBufferIdx = !activeBufferIdx;
		
		numChannels = std::min(numChannels, stereoBuffers[inactiveBufferIdx].size());
		for (int i = 0; i < numChannels; ++i){
			stereoBuffers[inactiveBufferIdx][i].resize(length);
		}
		std::array<float *const, 2> ptrsToWriteTo = {
			stereoBuffers[inactiveBufferIdx][0].data(),
			stereoBuffers[inactiveBufferIdx][1].data()
		};
		return ptrsToWriteTo;
	}

	inline void updateActive(){
		updateActiveBufferIndex();
		activeSpan = std::span<float>(stereoBuffers[activeBufferIdx][0]);
	}
	void setFileSampleRate(double sampleRate){
		fileSampleRate = sampleRate;
	}
	double const &getFileSampleRateRef() const {
		return fileSampleRate;
	}
private:
	std::array<stereoVector_t, 2> stereoBuffers;
	std::span<float> activeSpan;
	double fileSampleRate { 0.0 };
	unsigned int activeBufferIdx { 0 };
	inline void updateActiveBufferIndex(){
		activeBufferIdx = !activeBufferIdx;
		jassert((activeBufferIdx == 0) | (activeBufferIdx == 1));
	}

};
