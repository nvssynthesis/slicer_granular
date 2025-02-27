/*
  ==============================================================================

    dsp_util.h
    Created: 8 Aug 2023 3:22:30am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "sprout/math.hpp"
//#include "sprout/math/constants.hpp"
#include <span>

namespace nvs {
namespace util {

inline auto checkNan(float a){
	return a != a;
}
inline auto checkInf(float a){
	return std::isinf(a);
}
inline bool checkNanOrInf(std::span<float> sp) {
	for (auto const &a : sp){
		if (checkNan(a) || checkInf(a)){
			return true;
		}
	}
	return false;
}
template<size_t N>
inline bool checkNanOrInf(std::array<float, N> sp) {
	return checkNanOrInf(std::span<float>(sp));
}


inline float scale(float val, float min, float range){
	return (val - min) / range;
}

template<int minSemitones, int maxSemitones, int reso>
struct SemitonesRatioTable {
	constexpr static float semitoneRatio = static_cast<float>(1.059463094359295);
	
	constexpr SemitonesRatioTable()	:	values()
	{
		double incr = range() / static_cast<double>(reso);
		int i = 0;
		for (double x = static_cast<double>(minSemitones); x < static_cast<double>(maxSemitones);
			 x += incr)
		{
			values[i] = sprout::pow(semitoneRatio, static_cast<float>(x));
			++i;
		}
	}
	static constexpr double range(){
		return static_cast<double>(maxSemitones) - static_cast<double>(minSemitones);
	}
	std::array<float, reso> values;
	
	constexpr float operator()(float x){
		x = scale(x, minSemitones, range());
		float fidx = x * static_cast<float>(reso);
		int iidx = static_cast<int>(fidx);
		iidx = iidx >= 0 ? iidx : 0;
		iidx = iidx < reso ? iidx : reso-1;
		return values[iidx];
	}
};
static SemitonesRatioTable<-60, 60, 240> semitonesRatioTable;

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
}	// namespace util
}	// namespace nvs
