/*
  ==============================================================================

    dsp_util.h
    Created: 8 Aug 2023 3:22:30am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once

template<typename T>
class RMS {
	T val {0};
	size_t counter {0};
	
public:
	// accumulate square
	void accumulate(T x){
		x *= x;
		val += x;
		++counter;
	}
	// get rms
	T query(){
		auto ret = val / static_cast<T>(counter);
		ret = sqrt(ret);
		counter = 0;
		val = static_cast<T>(0);
		return ret;
	}
};

template<typename T, size_t length>
class CircularBuffer {
	std::array<T, length> buff;
	size_t write_idx {0};
	size_t getReadIdx() const {
		long read_idx = static_cast<long>(write_idx) + 1;
		if (read_idx == length){
			read_idx = 0;
		}
		return static_cast<size_t>(read_idx);
	}
public:
	CircularBuffer(){
		std::fill(buff.begin(), buff.end(), static_cast<T>(0));
	}
	T operator()(T x){
		auto const readIdx = getReadIdx();
		T const ret = buff[readIdx];
		
		buff[write_idx] = x;
		write_idx = readIdx;
		
		return ret;
	}
};

template<typename T, size_t length>
class AveragingBuffer {
	static constexpr T normalizer {static_cast<T>(1.0) / static_cast<T>(length)};
	std::array<T, length> buff;
	T runningAverage {static_cast<T>(0)};
	size_t write_idx {0};
	size_t getReadIdx() const {
		long read_idx = static_cast<long>(write_idx) + 1;
		if (read_idx == length){
			read_idx = 0;
		}
		return static_cast<size_t>(read_idx);
	}
public:
	AveragingBuffer(){
		static_assert(length > 0, "length must exceed 0.");
		std::fill(buff.begin(), buff.end(), static_cast<T>(0));
	}
	T operator()(T x){
		auto const readIdx = getReadIdx();
		runningAverage -= buff[write_idx] * normalizer;
		runningAverage += x * normalizer;

		buff[write_idx] = x;
		write_idx = readIdx;
		
		return runningAverage ;
	}
};

// more expensive; best to use with smaller lengths.
template<typename T, size_t length>
class WeightedAveragingBuffer {
	
	std::array<T, length> history;
	int buff_idx_offset {0};
	std::array<T, length> weights;
	T overallScalar {static_cast<T>(0)};
public:
	WeightedAveragingBuffer() {
		for (int i = 0; i < length; ++i){
			T const denom = static_cast<T>(i + 1.0);
			history[i] = 0.f;
			weights[i] = 1.f / denom;
			overallScalar += weights[i];
		}
		overallScalar = static_cast<T>(1.0) / overallScalar;
	}
	T operator()(T x){
		T ret = static_cast<T>(0.0);
		history[buff_idx_offset] = x;
		for (int i = 0; i < length; ++i){
			size_t buff_idx = buff_idx_offset + i;
			buff_idx %= length;
			ret += history[buff_idx] * weights[i];
		}
		--buff_idx_offset;
		if (buff_idx_offset < 0){
			buff_idx_offset += length;
		}
		
		return ret * overallScalar;
	}
};
