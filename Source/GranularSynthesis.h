/*
  ==============================================================================

    GranularSynthesis.h
    Created: 14 Jun 2023 10:28:50am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "nvs_libraries/include/nvs_gen.h"
#include "XoshiroCpp.hpp"
#include <span>

/*** TODO:
 -interpolation!
 -duration randomness must be improved (so sensitive)
 -duration should extend with position as a centrepoint, not startpoint
 -make duration independent of transpose
 -envelopes shall have secondary parameter, plateau, which clips the window before parzen
 -add automatic traversal
 */

namespace nvs {
namespace gran {

template<typename T>
struct RandomNumberGenerator {
private:
	XoshiroCpp::Xoshiro128Plus xosh;
public:
	RandomNumberGenerator()	:	xosh(1234567890UL){}
	T operator()(){
		std::uint32_t randomBits = xosh();
		float randomFloat = XoshiroCpp::FloatFromBits(randomBits);
		return randomFloat;
	}
};

struct genGrain1 {
private:
	nvs::gen::history<float> _histo;// history of 'busy' boolean signal, goes to [switch 1 2]
	nvs::gen::latch<float> _transposeLatch {1};	// latches transposition from gate on, goes toward dest windowing
	nvs::gen::latch<float> _durationLatch;	// latches duration from gate on, goes toward dest windowing
	nvs::gen::latch<float> _offsetLatch;// latches offset from gate on, goes toward dest windowing
	nvs::gen::latch<float> _skewLatch;
	nvs::gen::accum<float> _accum;	// accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
	
	nvs::gen::latch<float> _panLatch;
	
	float _transpRat = 1.f;
	float _duration = 1.f;
	float _offset = 0.f;
	float _skew = 0.5f;
	float _pan = 0.5f;
	
	float _transposeRand = 0.f;
	float _durationRand = 0.f;
	float _offsetRand = 0.f;
	float _skewRand = 0.f;
	float _panRand = 0.f;
	
//	vecReal const *_waveVec;
	std::span<float> const &_waveSpan;
	
	int grainId;
	// these pointers are set by containing granular synth
	size_t *const _numGrains_ptr;
	RandomNumberGenerator<float> *const _ng_ptr;
	
public:
	explicit genGrain1(std::span<float> const &waveSpan, RandomNumberGenerator<float> *const ng, size_t *const numGrains = nullptr, int newId = -1);

	void setId(int newId);
	void setTranspose(float semitones);
	void setDuration(float duration);
	void setOffset(float offset);
	void setSkew(float skew);
	void setPan(float pan);
	void setTransposeRand(float transposeRand);
	void setDurationRand(float durationRand); //('slope' in max patch)
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
	size_t _numGrains { 100 };
	std::vector<genGrain1> _grains;
	gen::phasor _phasorInternalTrig;
	nvs::gen::history<float> _speedHisto;
	nvs::gen::history<float> _durationHisto;
	nvs::gen::history<float> _offsetHisto;
	nvs::gen::history<float> _triggerHisto;
	nvs::gen::ramp2trig<float> _ramp2trig;
	
	nvs::gen::latch<float> _speedRandomLatch;
	float _speedRandomness {0.f};
		
	RandomNumberGenerator<float> _ng;
public:
	explicit genGranPoly1(float const &sampleRate, std::span<float> const &wavespan, size_t nGrains);
	
	void setTranspose(float transpositionRatio);
	void setPosition(float positionNormalized);
	void setSpeed(float newSpeed);
	void setDuration(float dur_ms);
	void setSkew(float skew);
	void setPan(float pan);
	void setTransposeRandomness(float randomness);
	void setPositionRandomness(float randomness);
	void setDurationRandomness(float randomness);
	void setSpeedRandomness(float randomness);
	void setSkewRandomness(float randomness);
	void setPanRandomness(float randomness);
	std::array<float, 2> operator()(float triggerIn);
};

}	// namespace granular
}	// namespace nvs
