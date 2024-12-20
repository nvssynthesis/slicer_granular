/*
  ==============================================================================

    GranularSynthesis.h
    Created: 14 Jun 2023 10:28:50am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <map>
#include <algorithm>
#include <numeric>

#include "../Random.h"
#include "../../nvs_libraries/nvs_libraries/include/nvs_gen.h"
#include <JuceHeader.h>

/*** TODO:
 -envelopes shall have secondary parameter, plateau, which clips the window before parzen
 -add automatic traversal
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
	float_t getMu() const {
		return _msp.mu;
	}
	float_t getSigma() const {
		return _msp.sigma;
	}
	nvs::rand::BoxMuller &_rng;
private:
	float_t val;
	MuSigmaPair<float_t> _msp;
};

//inline void logIfNaN(float f, juce::String const &&description, std::function<void(const juce::String&)> const &loggerFunc){
////	if (f != f){
////		loggerFunc("  " + description + " NaN");
////	}
////	if (std::isinf(f)){
////		loggerFunc("  " + description + " inf");
////	}
//}
//template<typename T>
//inline void logIfNull(T *t, juce::String const &&description, std::function<void(const juce::String&)> const &loggerFunc){
////	if (t == nullptr){
////		loggerFunc("  " + description + " nullptr");
////	}
//}

inline double millisecondsToSamples(double ms, double sampleRate) {
	return (ms / 1000.0) * sampleRate;
}
inline double millisecondsToHertz(double ms){
	return 1000.0 / ms;
}
inline double samplesToMilliseconds(double samps, double sampleRate) {
	return (samps / sampleRate) * 1000.0;
}

inline double millisecondsToFreqSamps(double ms, double sampleRate) {
	auto const samps = millisecondsToSamples(ms, sampleRate);
	return 1.0 / samps;
}
inline double durationMsToGaussianSpace(double ms, double sampleRate){
	(void)sampleRate;
	return millisecondsToHertz(ms);
}
inline double durationGaussianToProcessingSpace(double hertz, double sampleRate){
	// input is in hertz
	// = cycles per second
	// samples per second:
	return hertz / sampleRate;
}


class genGranPoly1 {
public:
	genGranPoly1(double const &sampleRate, juce::AudioBuffer<float>& waveBuffer, double const &fileSampleRate, unsigned long seed = 1234567890UL);
	virtual ~genGranPoly1() = default;
	//====================================================================================
	void setAudioBlock(juce::AudioBuffer<float>& waveBuffer);
	//====================================================================================
	inline void noteOn(noteNumber_t note, velocity_t velocity){	// reassign to noteHolder
		doNoteOn(note, velocity);
	}
	inline void noteOff(noteNumber_t note){						// remove from noteHolder
		doNoteOff(note);
	}
	inline void updateNotes(/*enum noteDistribution_t?*/){
		doUpdateNotes();
	}
	inline void clearNotes(){
		doClearNotes();
	}
	inline void shuffleIndices(){
		doShuffleIndices();
	}
	//=======================================================================
	inline void setTranspose(float transpositionRatio){
		doSetTranspose(transpositionRatio);
	}
	inline void setSpeed(float newSpeed){
		doSetSpeed(newSpeed);
	}
	inline void setPosition(float positionNormalized){
		doSetPosition(positionNormalized);
	}
	inline void setDuration(float dur_ms){
		doSetDuration(static_cast<double>(dur_ms));
	}
	inline void setSkew(float skew){
		doSetSkew(skew);
	}
	inline void setPlateau(float plateau){
		doSetPlateau(plateau);
	}
	inline void setPan(float pan){
		doSetPan(pan);
	}
	//=======================================================================
	inline void setTransposeRandomness(float randomness){
		doSetTransposeRandomness(randomness);
	}
	inline void setPositionRandomness(float randomness){
		doSetPositionRandomness(static_cast<double>(randomness));
	}
	inline void setDurationRandomness(float randomness){
		doSetDurationRandomness(static_cast<double>(randomness));
	}
	inline void setSpeedRandomness(float randomness){
		doSetSpeedRandomness(randomness);
	}
	inline void setSkewRandomness(float randomness){
		doSetSkewRandomness(randomness);
	}
	inline void setPlateauRandomness(float randomness){
		doSetPlateauRandomness(randomness);
	}
	inline void setPanRandomness(float randomness){
		doSetPanRandomness(randomness);
	}
	inline std::array<float, 2> operator()(float triggerIn){
		return doProcess(triggerIn);
	}
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
protected:
	double const &_sampleRate;				// dependent on owning instantiator, subject to change value from above
	juce::dsp::AudioBlock<float> _waveBlock;	// dependent on owning instantiator, subject to change address from above
	double const &_fileSampleRate;
	//================================================================================
	virtual void doNoteOn(noteNumber_t note, velocity_t velocity);	// reassign to noteHolder
	virtual void doNoteOff(noteNumber_t note);						// remove from noteHolder
	virtual void doUpdateNotes(/*enum noteDistribution_t?*/);
	virtual void doClearNotes();
	virtual void doShuffleIndices();
	
	virtual void doSetTranspose(float transpositionRatio);
	virtual void doSetSpeed(float newSpeed);
	virtual void doSetPosition(double positionNormalized);
	virtual void doSetDuration(double dur_ms);
	virtual void doSetSkew(float skew);
	virtual void doSetPlateau(float plateau);
	virtual void doSetPan(float pan);
	
	virtual void doSetTransposeRandomness(float randomness);
	virtual void doSetPositionRandomness(double randomness);
	virtual void doSetDurationRandomness(double randomness);
	virtual void doSetSpeedRandomness(float randomness);
	virtual void doSetSkewRandomness(float randomness);
	virtual void doSetPlateauRandomness(float randomness);
	virtual void doSetPanRandomness(float randomness);
	std::array<float, 2> doProcess(float triggerIn);
	
private:
	nvs::rand::BoxMuller _gaussian_rng;

	float _normalizer {1.f};
	std::vector<genGrain1> _grains;
	std::vector<size_t> _grainIndices;	// used to index grains in random order
	gen::phasor<double> _phasorInternalTrig;

	LatchedGaussianRandom<float> speed_lgr {_gaussian_rng, {1.f, 0.f}};
	
	nvs::gen::history<float> _triggerHisto;
	nvs::gen::ramp2trig<float> _ramp2trig;
	
	NoteHolder noteHolder {};
	
	std::function<void(const juce::String&)> logger = nullptr;
};

class genGrain1 {
public:
	explicit genGrain1(double const &sampleRate, juce::AudioBuffer<float>& waveBuffer, double const &fileSampleRate,
					   nvs::rand::BoxMuller *const gaussian_rng, int newId = -1);

	void setId(int newId);
	void setAudioBlock(juce::AudioBuffer<float>& audioBuffer);
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
	
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
private:
	double const &_sampleRate;
	juce::dsp::AudioBlock<float> _waveBlock;
	double const &_fileSampleRate;
	
	nvs::rand::BoxMuller *const _gaussian_rng_ptr;
	int grainId;
	
	nvs::gen::history<float> _busyHisto;// history of 'busy' boolean signal, goes to [switch 1 2]
	nvs::gen::latch<float> _ratioForNoteLatch {1.f};
	nvs::gen::latch<float> _amplitudeForNoteLatch {0.f};
	
	LatchedGaussianRandom<float> transpose_lgr;
	LatchedGaussianRandom<double> position_lgr;		// latches position from gate on, goes toward dest windowing
	LatchedGaussianRandom<double> duration_lgr;	// latches duration from gate on, goes toward dest windowing
	LatchedGaussianRandom<float> skew_lgr;
	LatchedGaussianRandom<float> plateau_lgr;
	LatchedGaussianRandom<float> pan_lgr;
		
	nvs::gen::accum<double> _accum;	// accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
		
	float _ratioBasedOnNote {1.f};	// =1.f. later this may change according to a settable concert pitch
	float _amplitudeBasedOnNote {0.f};
	
	std::function<void(const juce::String&)> logger = nullptr;
};

}	// namespace granular
}	// namespace nvs
