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
#include "../../nvs_libraries/nvs_libraries/include/nvs_LFO.h"
#include "GrainDescription.h"
#include "VoicesXGrains.h"
#include <JuceHeader.h>

/*** TODO:
 -optimize
 --tanh table
*/

namespace nvs {
namespace gran {

typedef int noteNumber_t;
typedef int velocity_t;
typedef std::map<noteNumber_t, velocity_t> NoteHolder;

class Grain;

inline double millisecondsToSamples(double ms, double sampleRate) {
	return (ms / 1000.0) * sampleRate;
}
inline double millisecondsToHertz(double ms) {
	return 1000.0 / ms;
}
inline double samplesToMilliseconds(double samps, double sampleRate) {
	return (samps / sampleRate) * 1000.0;
}
inline double millisecondsToFreqSamps(double ms, double sampleRate) {
	auto const samps = millisecondsToSamples(ms, sampleRate);
	return 1.0 / samps;
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
//========================================================================================================================================
struct GranularSynthSharedState {
	GranularSynthSharedState(juce::AudioProcessorValueTreeState &apvts)	:	_apvts(apvts){}
	double _playback_sample_rate {0.0};
	
	struct Buffer {
		juce::dsp::AudioBlock<float> _wave_block;
		double _file_sample_rate {0.0};
		size_t _filename_hash;
	};
	Buffer _buffer;
	
	std::function<void(const juce::String&)> _logger_func {nullptr};
	
	struct Settings {
		bool _center_position_at_env_peak { true };
		float _duration_pitch_compensation { 1.f };
		float _duration_dependence_on_read_bounds { 0.95f };// at 0, the 'duration' parameter is a fraction of the whole file; at 1, it is a fraction of the current event within the file.
	};
	Settings _settings;
	
	juce::AudioProcessorValueTreeState& _apvts;
};

struct GranularVoiceSharedState {
	// random generators are uniquely seeded per voice.
	BoxMuller _gaussian_rng;
	ExponentialRandomNumberGenerator _expo_rng;
	int _voice_id;
	
	float trigger;
	
	nvs::lfo::simple_lfo<float> _scanner;
	float _scanner_amount {0.f};
};

//========================================================================================================================================

struct ReadBounds {
	double begin {0.0};	// default normalized
	double end	 {0.0};	// default 0 length
	ReadBounds operator*(double mult) {
		return ReadBounds{begin * mult, end * mult};
	}
};

//========================================================================================================================================

struct GrainwisePostProcessing
{
	float operator()(float x);	// single channel
	
	std::array<float, 2> operator()(std::array<float, 2> x){	// apply single to both channels
		std::array<float, 2> retval {0.f, 0.f};
		for (size_t i = 0; i < x.size(); ++i){
			retval[i] = operator()(x[i]);
		}
		return retval;
	}
	float drive {1.0f};
	float makeup_gain {1.0f};
};

class PolyGrain {
public:
	PolyGrain(GranularSynthSharedState *const synth_shared_state,
			  GranularVoiceSharedState *const voice_shared_state);
	virtual ~PolyGrain() = default;
	//====================================================================================
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
	void setGrainsIdle();
	std::vector<float> getBusyStatuses() const;
	//=======================================================================
	inline std::array<float, 2> operator()(float triggerIn){
		return doProcess(triggerIn);
	}
	void setReadBounds(ReadBounds newReadBounds) ;
	struct WeightedReadBounds {
		ReadBounds bounds;
		double weight;
	};
	void setMultiReadBounds(std::vector<WeightedReadBounds> newReadBounds) ;
	std::vector<GrainDescription> getGrainDescriptions() const;
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
	
	void setParams();
protected:
	//================================================================================
	virtual void doNoteOn(noteNumber_t note, velocity_t velocity);	// reassign to noteHolder
	virtual void doNoteOff(noteNumber_t note);						// remove from noteHolder
	virtual void doUpdateNotes(/*enum noteDistribution_t?*/);
	virtual void doClearNotes();
	virtual void doShuffleIndices();

	std::array<float, 2> doProcess(float triggerIn);
	
	//================================================================================
	GranularSynthSharedState *const _synth_shared_state;
	GranularVoiceSharedState *const _voice_shared_state;

	std::vector<Grain> _grains;
private:
	float _normalizer {1.f};

    std::vector<size_t> _grain_indices;	// used to index grains in random order
    gen::phasor<double> _phasor_internal_trig;

	LatchedExponentialRandom_d _speed_ler; /*{_expo_rng, {1.f, 0.f}};*/
    
    nvs::gen::history<float> _trigger_histo;
    nvs::gen::ramp2trig<float> _ramp2trig;
    
    NoteHolder _note_holder {};
};

class Grain {
public:
	explicit Grain(GranularSynthSharedState *const synth_shared_state,
					   GranularVoiceSharedState *const voice_shared_state,
					   int newId = -1);
	
	void setId(int newId);
	void resetAccum();
	void setAccum(float newVal);
	void setRatioBasedOnNote(float ratioForNote);
	void setAmplitudeBasedOnNote(float velocity);
	
	float getBusyStatus() const;
	void setBusyStatus(bool newBusyStatus);
	struct outs {
		float next 		{0.f};
		float busy 		{0.f};
		float audio_L	{0.f};
		float audio_R 	{0.f};
	};
	
	void setReadBounds(ReadBounds newReadBounds);
	outs operator()(float const trig_in);
	
	GrainDescription getGrainDescription() const;
	
	void setFirstPlaythroughOfVoicesNote(bool isFirstPlaythrough){
		firstPlaythroughOfVoicesNote = isFirstPlaythrough;
	}
	void setParams();
private:
	GranularSynthSharedState *const _synth_shared_state;
	GranularVoiceSharedState *const _voice_shared_state;
	
	void writeToLog(const juce::String &s){
		assert(_synth_shared_state != nullptr);
		_synth_shared_state->_logger_func(s);
	}
	
	int _grain_id;
	
	// this is hacky and would be better implemented as a sort of latch as well
	bool wantsToDisableFirstPlaythroughOfVoicesNote {false};	// the signal to turn firstPlaythroughOfVoicesNote off
	bool firstPlaythroughOfVoicesNote { true };// the signal indicating that the currently set parameters, via latches/latched randoms, are invalid and thus the grain should be muted
	
	
    nvs::gen::history<float> _busy_histo; // history of 'busy' boolean signal, goes to [switch 1 2]
    nvs::gen::latch<float> _ratio_for_note_latch {1.f};
    nvs::gen::latch<float> _amplitude_for_note_latch {0.f};
	nvs::gen::latch<float> _scanner_for_position_latch {0.f};
    
	LatchedGaussianRandom_f 	_transpose_lgr;
	LatchedGaussianRandom_d 	_position_lgr; // latches position from gate on, goes toward dest windowing
	LatchedExponentialRandom_d 	_duration_ler; // latches duration from gate on, goes toward dest windowing
	LatchedGaussianRandom_f 	_skew_lgr;
	LatchedGaussianRandom_f 	_plateau_lgr;
	LatchedGaussianRandom_f 	_pan_lgr;
    
    nvs::gen::accum<double> _accum; // accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
    
	ReadBounds _normalized_read_bounds;// defaults to normalized read bounds. TSN variant can adjust effective read bounds (changing begin and end based on event positions/durations).
	ReadBounds _upcoming_normalized_read_bounds;
	
	GrainwisePostProcessing _postProcessing;
	
	// these get used both for operator() as well as passing on to gui via getGrainDescription
    double _sample_index {0.0};
    float _waveform_read_rate {0.0};
    float _window {0.f};
	float _pan {0.f};
    
    float _ratio_based_on_note {1.f}; // =1.f. later this may change according to a settable concert pitch
    float _amplitude_based_on_note {0.f};

	float _grain_drive {1.0f};
	float _grain_makeup_gain {1.0f};
};

}	// namespace granular
}	// namespace nvs
