/*
  ==============================================================================

    GranularSynthesis.h
    Created: 14 Jun 2023 10:28:50am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "OnsetAnalysis.h"
#include "nvs_libraries/include/nvs_gen.h"
#include "/Users/nicholassolem/development/Xoshiro-cpp/XoshiroCpp.hpp"
#include <span>

namespace nvs {
namespace gran {

using namespace nvs::analysis;

#if 0
inline void loadOnsetsSamples(waveHolder &wh, std::vector<size_t> onsetsSamples){
	assert(onsetsSamples.back() <= (wh.waveform.size()));
	wh.onsets = onsetsSamples;
}
inline void loadOnsetsSeconds(waveHolder &wh, vecReal onsetsSeconds, float sampleRate){
	size_t N = onsetsSeconds.size();
	std::vector<size_t> onsetsSamples(N);
	for (auto i = 0; i < N; ++i){
		onsetsSamples[i] = onsetsSeconds[i] * sampleRate;
		size_t prev = std::max(0, i - 1);
		assert(onsetsSamples[i] >= onsetsSamples[prev]);
	}
	loadOnsetsSamples(wh, onsetsSamples);
}
#endif

template<typename T, typename string_t = char>
struct occasionalPrinter {
	size_t count { 10 };
	size_t current {0};
	occasionalPrinter(size_t c){
		count = c;
	}
	void operator()(T x, string_t s = '\n'){
		if (current == 0)
			std::cout << x << s;
		++current;
		if (current >= count){
			current = 0;
		}
	}
};

template<typename T>
struct numberGenerator {
private:
	T state = 0.25f;
	XoshiroCpp::Xoshiro128Plus xosh;
public:
	numberGenerator()	:	xosh(1234567890UL){}
	T operator()(){
		// next random float
		state += 0.91f;
		if ((state > 0.5) && (state > 0.6)){
			state -= 0.123f;
		}
		state = fmod(state, 1.0);
		
		std::uint32_t randomBits = xosh();
		float randomFloat = XoshiroCpp::FloatFromBits(randomBits);
		return (state * 0.f) + (randomFloat * 1.f);
	}
};

struct genGrain1 {
private:
	nvs::gen::history<float> _histo;// history of 'busy' boolean signal, goes to [switch 1 2]
	nvs::gen::latch<float> _slopeLatch;	// latches slope from gate on, goes toward dest windowing
	nvs::gen::latch<float> _offsetLatch;// latches offset from gate on, goes toward dest windowing
	nvs::gen::latch<float> _skewLatch;
	nvs::gen::accum<float> _accum;	// accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
	
	nvs::gen::latch<float> _panLatch;
	
	float _slope = 1.f;
	float _offset = 0.f;
	float _skew = 0.5f;
	float _pan = 0.5f;
	
	float _slopeRand = 0.f;
	float _offsetRand = 0.f;
	float _skewRand = 0.f;
	float _panRand = 0.f;
	
//	vecReal const *_waveVec;
	std::span<float> const &_waveSpan;
	
	int grainId;
	// these pointers are set by containing granular synth
	size_t *const _numGrains_ptr;
	numberGenerator<float> *const _ng_ptr;
	
	occasionalPrinter<float, std::string> printer;
public:
	explicit genGrain1(std::span<float> const &waveSpan, numberGenerator<float> *const ng, size_t *const numGrains = nullptr, int newId = -1);

	void setId(int newId);
	void setSlope(float slope);
	void setOffset(float offset);
	void setSkew(float skew);
	void setPan(float pan);
	void setSlopeRand(float slopeRand);
	void setOffsetRand(float offsetRand);
	void setSkewRand(float skewRand);
	void setPanRand(float panRand);
	struct outs {
		float next;
		float busy;
		float audio;
		float audio_R;
	};
	
	outs operator()(float trig_in);
};

struct genGranPoly1 {
private:
	float const &_sampleRate;				// dependent on owning instantiator, subject to change value from above
	std::span<float> const &_wavespan;	// dependent on owning instantiator, subject to change address from above
	size_t _numGrains { 16 };
	std::vector<genGrain1> _grains;
	gen::phasor _phasorInternalTrig;
	nvs::gen::history<float> _rateHisto;
	nvs::gen::history<float> _slopeHisto;
	nvs::gen::history<float> _offsetHisto;
	nvs::gen::history<float> _triggerHisto;
	nvs::gen::ramp2trig<float> _ramp2trig;
	
	nvs::gen::latch<float> _rateRandomLatch;
	float _rateRandomness {0.f};
	
	nvs::gen::change<float> change_test;
	
	numberGenerator<float> _ng;
	
public:
	explicit genGranPoly1(float const &sampleRate, std::span<float> const &wavespan, size_t nGrains = 16);
	void setPosition(float positionNormalized);
	void setRate(float newRate);
	void setDuration(float dur_ms);
	void setSkew(float skew);
	void setPan(float pan);
	void setPositionRandomness(float randomness);
	void setSpeedRandomness(float randomness);
	void setPanRandomness(float randomness);
	std::array<float, 2> operator()(float triggerIn);
};

}	// namespace granular
}	// namespace nvs
