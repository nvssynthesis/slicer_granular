/*
  ==============================================================================

    GranularSynthesis.h
    Created: 14 Jun 2023 10:28:50am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <span>
#include <map>
#include <algorithm>
#include <numeric>
#include "XoshiroCpp.hpp"
#include "nvs_libraries/include/nvs_gen.h"

/*** TODO:
 -envelopes shall have secondary parameter, plateau, which clips the window before parzen
 -add automatic traversal
 
 -simplify some aspects by making RandomizableParameter class
 -instead of having randomness act positively or negatively in a linear fashion, it shall be a PDF
	-gaussian, white, cauchy, inverse gaussian perhaps
 
*/

namespace nvs {
namespace gran {

template<typename T>
struct RandomNumberGenerator {
public:
	RandomNumberGenerator()	:	xosh(1234567890UL){}
	T operator()(){
		std::uint32_t randomBits = xosh();
		float randomFloat = XoshiroCpp::FloatFromBits(randomBits);
		return randomFloat;
	}
	XoshiroCpp::Xoshiro128Plus &getGenerator() {
		return xosh;
	}
private:
	XoshiroCpp::Xoshiro128Plus xosh;
};

typedef int noteNumber_t;
typedef int velocity_t;
typedef std::map<noteNumber_t, velocity_t> NoteHolder;

struct genGrain1;

struct genGranPoly1 {
public:
	genGranPoly1(float const &sampleRate, std::span<float> const &wavespan, size_t nGrains);
	
	void noteOn(noteNumber_t note, velocity_t velocity);	// reassign to noteHolder
	void noteOff(noteNumber_t note);						// remove from noteHolder
	void updateNotes(/*enum noteDistribution_t?*/);
	void shuffleIndices();
	
	void setTranspose(float transpositionRatio);
	void setSpeed(float newSpeed);
	inline void setPosition(float positionNormalized){
		setPosition(static_cast<double>(positionNormalized));
	}
	void setPosition(double positionNormalized);
	void setDuration(float dur_ms){
		setDuration(static_cast<double>(dur_ms));
	}
	void setDuration(double dur_ms);
	void setSkew(float skew);
	void setPlateau(float plateau);
	void setPan(float pan);
	void setTransposeRandomness(float randomness);
	void setPositionRandomness(float randomness){
		setPositionRandomness(static_cast<double>(randomness));
	}
	void setPositionRandomness(double randomness);
	void setDurationRandomness(float randomness){
		setDurationRandomness(static_cast<double>(randomness));
	}
	void setDurationRandomness(double randomness);
	void setSpeedRandomness(float randomness);
	void setSkewRandomness(float randomness);
	void setPlateauRandomness(float randomness);
	void setPanRandomness(float randomness);
	std::array<float, 2> operator()(float triggerIn);
	
protected:
	float const &_sampleRate;				// dependent on owning instantiator, subject to change value from above
	std::span<float> const &_wavespan;	// dependent on owning instantiator, subject to change address from above
	size_t _numGrains;
private:
	float _normalizer {1.f};
	std::vector<genGrain1> _grains;
	std::vector<size_t> _grainIndices;	// used to index grains in random order
	gen::phasor _phasorInternalTrig;
	nvs::gen::history<float> _speedHisto;
	nvs::gen::history<double> _durationHisto;
	nvs::gen::history<double> _offsetHisto;
	nvs::gen::history<float> _triggerHisto;
	nvs::gen::ramp2trig<float> _ramp2trig;
	
	nvs::gen::latch<float> _speedRandomLatch;
	float _speedRandomness {0.f};
		
	RandomNumberGenerator<float> _rng;
	NoteHolder noteHolder {};
	
	inline double durationMsToFreqSamps(double dur_ms){
		dur_ms = nvs::memoryless::clamp_low<double>(dur_ms, 0.0);
		auto dur_samps = (dur_ms / 1000.0) * static_cast<double>(_sampleRate);
		
		dur_samps = nvs::memoryless::clamp_low<double>(dur_samps, 1.0);
		float freq_samps = 1.0 / dur_samps;
		return freq_samps;
	}
};

struct genGrain1 {
public:
	explicit genGrain1(std::span<float> const &waveSpan, RandomNumberGenerator<float> *const ng, size_t *const numGrains = nullptr, int newId = -1);

	void setId(int newId);
	void setRatioBasedOnNote(float ratioForNote);
	void setAmplitudeBasedOnNote(float velocity);
	void setTranspose(float semitones);
	void setDuration(double duration);
	void setOffset(double offset);
	void setSkew(float skew);
	void setPlateau(float plateau);
	void setPan(float pan);
	void setTransposeRand(float transposeRand);
	void setDurationRand(double durationRand); //('slope' in max patch)
	void setOffsetRand(double offsetRand);
	void setSkewRand(float skewRand);
	void setPlateauRand(float plateauRand);
	void setPanRand(float panRand);
	struct outs {
		float next;
		float busy;
		float audio;
		float audio_R;
	};
	
	outs operator()(float trig_in);
private:
	nvs::gen::history<float> _histo;// history of 'busy' boolean signal, goes to [switch 1 2]
	nvs::gen::latch<float> _ratioForNoteLatch {1.f};
	nvs::gen::latch<float> _amplitudeForNoteLatch {0.f};
	nvs::gen::latch<float> _transposeLatch {1.f};	// latches transposition from gate on, goes toward dest windowing
	nvs::gen::latch<float> _durationLatch;	// latches duration from gate on, goes toward dest windowing
	nvs::gen::latch<float> _offsetLatch;// latches offset from gate on, goes toward dest windowing
	nvs::gen::latch<float> _skewLatch;
	nvs::gen::latch<float> _plateauLatch {1.f};
	nvs::gen::accum<double> _accum;	// accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
	
	nvs::gen::latch<float> _panLatch;
	
	float _ratioBasedOnNote {1.f};	// =1.f. later this may change according to a settable concert pitch
	float _amplitudeBasedOnNote {0.f};
	
	float _transpRat = 1.f;
	double _duration = 1.0;
	double _offset = 0.0;
	float _skew = 0.5f;
	float _plateau = 1.f;
	float _pan = 0.5f;
	
	float _transposeRand = 0.f;
	double _durationRand = 0.0;
	double _offsetRand = 0.0;
	float _skewRand = 0.f;
	float _platRand = 0.f;
	float _panRand = 1.f;
	
//	vecReal const *_waveVec;
	std::span<float> const &_waveSpan;
	
	int grainId;
	// these pointers are set by containing granular synth
	size_t *const _numGrains_ptr;
	RandomNumberGenerator<float> *const _rng_ptr;
};

}	// namespace granular
}	// namespace nvs
