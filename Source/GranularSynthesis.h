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

#include "Random.h"
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

typedef int noteNumber_t;
typedef int velocity_t;
typedef std::map<noteNumber_t, velocity_t> NoteHolder;

struct genGrain1;

template<std::floating_point float_t>
struct MuSigmaPair {
	float_t mu;
	float_t sigma;
};

template<std::floating_point float_t>
struct LatchedGaussianRandom {
	LatchedGaussianRandom(nvs::rand::BoxMuller &rng, MuSigmaPair<float_t> msp)
	:	_rng(rng), val(msp.mu), _msp(msp){}
	
	float_t operator()(bool gate){
		if (gate){
			val = static_cast<float_t>(
										_rng(static_cast<double>(_msp.mu), static_cast<double>(_msp.sigma))
									   );
		}
		return val;
	}
	void setMu(float_t mu){
		_msp.mu = mu;
	}
	void setSigma(float_t sigma){
		_msp.sigma = sigma;
	}
	nvs::rand::BoxMuller &_rng;
private:
	float_t val;
	MuSigmaPair<float_t> _msp;
};

inline double durationMsToFreqSamps(double dur_ms, double sampleRate) {
	dur_ms = nvs::memoryless::clamp_low<double>(dur_ms, 0.0);
	auto dur_samps = (dur_ms / 1000.0) * sampleRate;
	
	dur_samps = nvs::memoryless::clamp_low<double>(dur_samps, 1.0);
	double freq_samps = 1.0 / dur_samps;
	return freq_samps;
}

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
	nvs::rand::BoxMuller _gaussian_rng;

	float _normalizer {1.f};
	std::vector<genGrain1> _grains;
	std::vector<size_t> _grainIndices;	// used to index grains in random order
	gen::phasor _phasorInternalTrig;

	LatchedGaussianRandom<float> speed_lgr {_gaussian_rng, {1.f, 0.f}};
	
	nvs::gen::history<double> _durationHisto;
	nvs::gen::history<double> _positionHisto;
	nvs::gen::history<float> _triggerHisto;
	nvs::gen::ramp2trig<float> _ramp2trig;
	
//	nvs::gen::latch<float> _speedRandomLatch;
		
//	RandomNumberGenerator _rng;
	NoteHolder noteHolder {};
};

struct genGrain1 {
public:
	explicit genGrain1(std::span<float> const &waveSpan, nvs::rand::BoxMuller *const gaussian_rng, size_t *const numGrains, int newId = -1);

	void setId(int newId);
	void setRatioBasedOnNote(float ratioForNote);
	void setAmplitudeBasedOnNote(float velocity);
	void setTranspose(float semitones);
	void setDuration(double duration);
	void setPosition(double position);
	void setSkew(float skew);
	void setPlateau(float plateau);
	void setPan(float pan);
	void setTransposeRand(float transposeRand);
	void setDurationRand(double durationRand); //('slope' in max patch)
	void setPositionRand(double positionRand);
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
	std::span<float> const &_waveSpan;
	nvs::rand::BoxMuller *const _gaussian_rng_ptr;
	int grainId;
	// these pointers are set by containing granular synth
	size_t *const _numGrains_ptr;
	
	nvs::gen::history<float> _busyHisto;// history of 'busy' boolean signal, goes to [switch 1 2]
	nvs::gen::latch<float> _ratioForNoteLatch {1.f};
	nvs::gen::latch<float> _amplitudeForNoteLatch {0.f};
//	nvs::gen::latch<float> _transposeLatch {1.f};	// latches transposition from gate on, goes toward dest windowing
	
	LatchedGaussianRandom<float> transpose_lgr;
	LatchedGaussianRandom<double> position_lgr;		// latches position from gate on, goes toward dest windowing
	LatchedGaussianRandom<double> duration_lgr;	// latches duration from gate on, goes toward dest windowing
	LatchedGaussianRandom<float> skew_lgr;
	LatchedGaussianRandom<float> plateau_lgr;
	LatchedGaussianRandom<float> pan_lgr;
		
	nvs::gen::accum<double> _accum;	// accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
		
	float _ratioBasedOnNote {1.f};	// =1.f. later this may change according to a settable concert pitch
	float _amplitudeBasedOnNote {0.f};
};

}	// namespace granular
}	// namespace nvs
