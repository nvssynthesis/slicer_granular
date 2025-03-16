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

template <typename float_t>
inline float_t tanh_pade_3_2(float_t x) {
	auto const x2 = x * x;
	return x * (15.0 + x2) / (15.0 + 6.0 * x2);
}
template <typename float_t>
inline float_t tanh_pade_5_6(float_t x) {
	auto const x2 = x * x;
	auto const x4 = x2 * x2;
	auto const x6 = x2 * x4;
	return x * (10395.0 + 1260.0 * x2 +21.0 * x4) / (10395.0 + 4725.0 * x2 + 210.0 * x4 + x6);
}

template <typename float_t>
inline float_t tanh_pade_7_6(float_t x){
	auto const x2 = x * x;
	auto const x4 = x2 * x2;
	auto const x6 = x2 * x4;
	return x * (135135.0 + 17325.0 * x2 + 378.0 * x4 + x6) / (135135.0 + 62370.0 * x2 + 3150.0 * x4 + 28.0 * x6);
}

template <typename float_t>
inline float_t log_pade(float_t x) {
	assert (x > -1.f);
	return juce::dsp::FastMathApproximations::logNPlusOne(x - 1.f);
}
inline double exp2_pade_22(double x){
	assert (x >= 0.0);
	auto x2 = x*x;
	auto num = 1.0 + 0.34657359 * x + 0.04003775 * x2;
	auto den = 1.0 + -0.34657359 * x + 0.04003775 * x2;
	return num / den;
}
inline double exp2_pade_33(double x){
	assert (x >= 0.0);
	double x2 = x*x;
	double x3 = x * x2;
	double num = 1.0 + 0.34657359*x + 0.0480453*x2 + 0.00277521*x3;
	double den = 1.0 + -0.34657359*x + 0.0480453*x2 + -0.00277521*x3;
	return num / den;
}

inline auto mod(double n, double d) {
	return fmod(fmod(n, d) + d, d);
}

// works for all x, but doesn't handle overflow properly
inline double exp2_mod(double x)
{
	auto const frac = mod(x, 1.0);
	auto const pade = exp2_pade_22(frac);
	
	auto const fl = (int) floor(x);
	auto res = scalbn(pade, fl);
	return res;
}

inline double exp_opt(double x) {
	constexpr double ln2 = sprout::log(2.0);
	static_assert (ln2 > 0.6931471);
	static_assert (ln2 < 0.6931472);
	constexpr double inv_ln2 = 1.0 / ln2;
	
	return exp2_mod(x * inv_ln2);
}

template <typename float_t>
inline float_t pow_pade(float_t base, float_t expo){
	if (base == 0.0){
		return 0.0;
	}
	assert (expo > 0.f);
	assert (expo == expo);
	assert (!std::isinf(expo));
	
	auto const lnx = log(base);
	assert (lnx == lnx);
	assert (!std::isinf(lnx));
	assert (lnx < 6.0);
	
	auto const y = exp_opt(lnx * expo);
	assert (y == y);
	assert (!std::isinf(y));
	return y;
}
template <typename float_t, float_t base>
inline float_t pow_fixed_base(float_t expo){
	// pow(x, y) = exp(ln(x) * y)
	constexpr float_t lnx = log(base);
	return exp_pade(lnx * expo);
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
