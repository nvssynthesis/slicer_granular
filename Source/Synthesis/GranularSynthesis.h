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

#include "../LatchedRandom.h"
#include "../../nvs_libraries/nvs_libraries/include/nvs_gen.h"
#include "GrainDescription.h"
#include <JuceHeader.h>

/*** TODO:
 -optimize
 --tanh table
*/

static constexpr size_t N_GRAINS =
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
								10;
#else
								15;
#endif

namespace nvs {
namespace gran {

typedef int noteNumber_t;
typedef int velocity_t;
typedef std::map<noteNumber_t, velocity_t> NoteHolder;

struct genGrain1;

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
/**
 Making use of concepts to guarantee common interface between latched random number generator types without inheritance (thus without virtual function calls)
 */
using MuSigmaPair_f  = nvs::rand::MuSigmaPair<float>;
using MuSigmaPair_d  = nvs::rand::MuSigmaPair<double>;
using BoxMuller = nvs::rand::BoxMuller;
using ExponentialRandomNumberGenerator = nvs::rand::ExponentialRandomNumberGeneratorWithVariance;
using LatchedGaussianRandom_f = decltype(createLatchedGaussianRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair_f>()));
using LatchedGaussianRandom_d = decltype(createLatchedGaussianRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair_d>()));
using LatchedExponentialRandom_f = decltype(createLatchedExponentialRandom(std::declval<ExponentialRandomNumberGenerator&>(), std::declval<MuSigmaPair_f>()));
using LatchedExponentialRandom_d = decltype(createLatchedExponentialRandom(std::declval<ExponentialRandomNumberGenerator&>(), std::declval<MuSigmaPair_d>()));

class genGranPoly1 {
public:
	genGranPoly1(unsigned long seed = 1234567890UL);
	virtual ~genGranPoly1() = default;
	//====================================================================================
	virtual void setAudioBlock(juce::dsp::AudioBlock<float> waveBlock, double fileSampleRate);
	void setSampleRate(double sampleRate);
	//====================================================================================
	static constexpr size_t getNumGrains(){
		return N_GRAINS;
	}
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
	std::vector<float> getBusyStatuses() const;
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
	std::vector<GrainDescription> getGrainDescriptions() const;
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
protected:
	juce::dsp::AudioBlock<float> _wave_block;	// dependent on owning instantiator, subject to change address from above
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
    BoxMuller _gaussian_rng;
	ExponentialRandomNumberGenerator _expo_rng;

    float _normalizer {1.f};
protected:
    std::vector<genGrain1> _grains;
private:
    std::vector<size_t> _grain_indices;	// used to index grains in random order
    gen::phasor<double> _phasor_internal_trig;

	LatchedExponentialRandom_d _speed_ler {_expo_rng, {1.f, 0.f}};
    
    nvs::gen::history<float> _trigger_histo;
    nvs::gen::ramp2trig<float> _ramp2trig;
    
    NoteHolder _note_holder {};
    
    std::function<void(const juce::String&)> _logger = nullptr;
};

class genGrain1 {
public:
	explicit genGrain1(BoxMuller *const gaussian_rng, ExponentialRandomNumberGenerator *const expo_rng, int newId = -1);
	
	void setId(int newId);
	void setAudioBlock(juce::dsp::AudioBlock<float> audioBlock, double fileSampleRate);
	void setSampleRate(double sampleRate);
	void setRatioBasedOnNote(float ratioForNote);
	void setAmplitudeBasedOnNote(float velocity);
	void setTranspose(float semitones);
	void setDuration(double duration);
	void setPosition(double position); 
	void setSkew(float skew);
	void setPlateau(float plateau);
	void setPan(float pan);
	void setTransposeRand(float transposeRand);
	void setDurationRand(double durationRand);
	void setPositionRand(double positionRand);
	void setSkewRand(float skewRand);
	void setPlateauRand(float plateauRand);
	void setPanRand(float panRand);
	float getBusyStatus() const;
	struct outs {
		float next 		{0.f};
		float busy 		{0.f};
		float audio_L	{0.f};
		float audio_R 	{0.f};
	};
	
	outs operator()(float const trig_in);
	
	GrainDescription getGrainDescription() const;
	
	void setLogger(std::function<void(const juce::String&)> loggerFunction);

protected:
    double _playback_sample_rate;
    juce::dsp::AudioBlock<float> _wave_block;
    double _file_sample_rate;
    
    BoxMuller *const _gaussian_rng_ptr;
    int _grain_id;
    
    nvs::gen::history<float> _busy_histo; // history of 'busy' boolean signal, goes to [switch 1 2]
    nvs::gen::latch<float> _ratio_for_note_latch {1.f};
    nvs::gen::latch<float> _amplitude_for_note_latch {0.f};
    
	LatchedGaussianRandom_f 	_transpose_lgr;
	LatchedGaussianRandom_d 	_position_lgr; // latches position from gate on, goes toward dest windowing
	LatchedExponentialRandom_d 	_duration_ler; // latches duration from gate on, goes toward dest windowing
	LatchedGaussianRandom_f 	_skew_lgr;
	LatchedGaussianRandom_f 	_plateau_lgr;
	LatchedGaussianRandom_f 	_pan_lgr;
    
    nvs::gen::accum<double> _accum; // accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
    
    double _sample_index {0.0};
    float _waveform_read_rate {0.0};
    float _window {0.f};
	float _pan {0.f};
    
    float _ratio_based_on_note {1.f}; // =1.f. later this may change according to a settable concert pitch
    float _amplitude_based_on_note {0.f};
    
    std::function<void(const juce::String&)> _logger = nullptr;
};

}	// namespace granular
}	// namespace nvs
