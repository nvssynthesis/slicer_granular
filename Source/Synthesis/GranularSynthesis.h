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
#include <JuceHeader.h>

#include "GrainDescription.h"
#include "VoicesXGrains.h"
#include "../Random/LatchedRandom.h"
#include "../../nvs_libraries/nvs_libraries/include/nvs_gen.h"
#include "../../nvs_libraries/nvs_libraries/include/nvs_LFO.h"
#include "juce_utils.h"

/*** TODO:
 -optimize
 --tanh table
*/

namespace nvs::gran {

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
using MuSigmaPair_f  = rand::MuSigmaPair<float>;
using MuSigmaPair_d  = rand::MuSigmaPair<double>;
using BoxMuller = rand::BoxMuller;
using ExponentialRandomNumberGenerator = rand::ExponentialRandomNumberGeneratorWithVariance;
using LatchedGaussianRandom_f = decltype(createLatchedGaussianRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair_f>()));
using LatchedGaussianRandom_d = decltype(createLatchedGaussianRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair_d>()));
using LatchedLogNormalRandom_f = decltype(createLatchedLogNormalRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair_f>()));
using LatchedLogNormalRandom_d = decltype(createLatchedLogNormalRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair_d>()));
using LatchedExponentialRandom_f = decltype(createLatchedExponentialRandom(std::declval<ExponentialRandomNumberGenerator&>(), std::declval<MuSigmaPair_f>()));
using LatchedExponentialRandom_d = decltype(createLatchedExponentialRandom(std::declval<ExponentialRandomNumberGenerator&>(), std::declval<MuSigmaPair_d>()));
//========================================================================================================================================
struct GranularSynthSharedState {
	explicit GranularSynthSharedState(AudioProcessorValueTreeState &apvts) :
        _buffer(), _apvts(apvts) {
    }

	double _playback_sample_rate {0.0};
	
	struct Buffer {
		juce::dsp::AudioBlock<float> _wave_block;   // NOLINT
	    std::vector<float> _loudness_profile;
		double _file_sample_rate {0.0};
		juce::int64 _audio_hash {0};                // NOLINT
	};
	Buffer _buffer;

	std::function<void(const juce::String&)> _logger_func {nullptr};   // NOLINT
	
	struct Settings {
	    bool _center_position_at_env_peak { true };
		float _duration_pitch_compensation { 1.f };
	};
	Settings _settings;

    double _concertPitchHz { 440.0 };
    double _notesPerOctave { 12.0 };

	AudioProcessorValueTreeState& _apvts;
};

struct GranularVoiceSharedState {
	// random generators are uniquely seeded per voice.
	BoxMuller _gaussian_rng;
	int _voice_id;

    // without this, the grain params won't update upon noteOn messages, leading to e.g. repeat of last note's pitch at beginning of note.
	bool forceGrainTrigger; // set to true on startNote, re-set to false after all grains are processed once.

    double grain_rate_hz;

    struct Scanner {
        lfo::simple_lfo<float> lfo;
        float shape {0.f};
    	float amount {0.f};
    } _scanner;
};

//========================================================================================================================================

struct ReadBounds {
	double begin {0.0};	// default normalized
	double end	 {0.0};	// default 0 length
	ReadBounds operator*(const double mult) const {
		return ReadBounds{begin * mult, end * mult};
	}
};

//========================================================================================================================================

class GrainwisePostProcessing
{
public:
    GrainwisePostProcessing(GranularSynthSharedState *synth_shared_state, GranularVoiceSharedState *voice_shared_state);

	std::array<float, 2> process(std::array<float, 2> x, double fractionalSample) const; // apply single to both channels
    void setNormalization(float norm);
    void setDriveMu(float muDb);					// linear gain (drive param is stored/consumed in linear gain, mapped from dB)
    void setDriveSigma(float sigmaDb);				// standard deviation, in dB, of the per-grain drive spread around the mu
    void updateDrive(bool shouldOpenLatches);	// per-grain latched randomization of drive
    void setMakeupGain(float gain);
private:
    float processChannel(float x, double t) const;	// single channel

    float _normalization {0.f};
	float _drive {1.0f};
	float _makeup_gain {1.0f};
	LatchedGaussianRandom_f _drive_lgr; // latches per-grain drive multiplier from gate on

    GranularSynthSharedState *_synth_shared_state;
};

class PolyGrain {
public:
	PolyGrain(GranularSynthSharedState *synth_shared_state,
			  GranularVoiceSharedState *voice_shared_state);
	virtual ~PolyGrain() = default;
	//====================================================================================
	void setSampleRate(double sampleRate);
	//====================================================================================
	static constexpr size_t getNumGrains(){
		return N_GRAINS;
	}
	void noteOn(const noteNumber_t note, const velocity_t velocity){	// reassign to noteHolder
		doNoteOn(note, velocity);
	}
	void noteOff(noteNumber_t note){						// remove from noteHolder
		doNoteOff(note);
	}
	void updateNotes(/*enum noteDistribution_t?*/){
		doUpdateNotes();
	}
	void clearNotes(){
		doClearNotes();
	}
	void shuffleIndices(){
		doShuffleIndices();
	}
	void setGrainsIdle();
	std::vector<float> getBusyStatuses() const;
	//=======================================================================

    std::array<float, 2> doProcess(float triggerIn);

	void setReadBounds(ReadBounds newReadBounds) ;
	struct WeightedReadBounds {
		ReadBounds bounds;
		double weight;
		WeightedReadBounds(ReadBounds b, double w)	:	bounds(b), weight(w) {}
	};
	void setEvents(const std::vector<WeightedReadBounds> &newWeightedReadBounds, const std::array<float, 3> &fundamental_frequencies) ;
	std::vector<GrainDescription> getGrainDescriptions() const;
	void setLogger(const std::function<void(const String&)> &loggerFunction) const;
	
	void setParams();
protected:
	//================================================================================
	virtual void doNoteOn(noteNumber_t note, velocity_t velocity);	// reassign to noteHolder
	virtual void doNoteOff(noteNumber_t note);						// remove from noteHolder
	virtual void doUpdateNotes(/*enum noteDistribution_t?*/);
	virtual void doClearNotes();
	virtual void doShuffleIndices();

	//================================================================================
	GranularSynthSharedState *const _synth_shared_state;
	GranularVoiceSharedState *const _voice_shared_state;

	std::vector<Grain> _grains;
private:
	float _normalizer {1.f};

    std::vector<size_t> _grain_indices;	// used to index grains in random order
    gen::phasor<double> _phasor_internal_trig;

	LatchedLogNormalRandom_d _speed_lnr; /*{_expo_rng, {1.f, 0.f}};*/
    
    gen::history<float> _trigger_histo;
    gen::ramp2trig<float> _ramp2trig;
    
    NoteHolder _note_holder {};
};

class Grain {
public:
	explicit Grain(GranularSynthSharedState *synth_shared_state,
					   GranularVoiceSharedState *voice_shared_state,
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
	void setWeight(const double w) {
		_grain_weight = static_cast<float>(w);
	}
    void setUnderlyingFundamentalFrequency(float midi_f0);
	outs operator()(float trig_in);
	
	GrainDescription getGrainDescription() const;
	
	void setFirstPlaythroughOfVoicesNote(const bool isFirstPlaythrough){
		firstPlaythroughOfVoicesNote = isFirstPlaythrough;
	}
	void setParams();
private:
	GranularSynthSharedState *const _synth_shared_state;
	GranularVoiceSharedState *const _voice_shared_state;
	
	void writeToLog(const String &s) {
		assert(_synth_shared_state != nullptr);
		_synth_shared_state->_logger_func(s);
	}

// #ifdef DBG
// 	std::unique_ptr<nvs::util::TimedPrinter> _timed_printer;
// #endif
	
	int _grain_id;
	
	// this is hacky and would be better implemented as a sort of latch as well
	bool wantsToDisableFirstPlaythroughOfVoicesNote {false};	// the signal to turn firstPlaythroughOfVoicesNote off
	bool firstPlaythroughOfVoicesNote { true };// the signal indicating that the currently set parameters, via latches/latched randoms, are invalid and thus the grain should be muted
	
	
    gen::history<float> _busy_histo; // history of 'busy' boolean signal, goes to [switch 1 2]
    gen::latch<double> _grain_rate_latch {1.f};
    gen::latch<float> _ratio_for_note_latch {1.f};
    gen::latch<float> _amplitude_for_note_latch {0.f};
	gen::latch<float> _scanner_for_position_latch {0.f};
	gen::latch<float> _grain_weight_latch {1.f}; // the weight based on distance to target point
    gen::latch<bool> _pitchify_latch {false};
    gen::latch<float> _underlying_f0_latch {0.f};   // for pitch compensation. if non-positive, it will have no effect.
    
	LatchedGaussianRandom_f 	_transpose_lgr;
	LatchedGaussianRandom_d 	_position_lgr; // latches position from gate on, goes toward dest windowing
	LatchedLogNormalRandom_d 	_density_lnr; // latches duration from gate on, goes toward dest windowing
	LatchedGaussianRandom_f 	_skew_lgr;
	LatchedGaussianRandom_f 	_plateau_lgr;
	LatchedGaussianRandom_f 	_pan_lgr;

    gen::accum<double> _accum; // accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
    
	ReadBounds _normalized_read_bounds;// defaults to normalized read bounds. TSN variant can adjust effective read bounds (changing begin and end based on event positions/durations).
	ReadBounds _upcoming_normalized_read_bounds;
	
	GrainwisePostProcessing _postProcessing;
	
	// these get used both for operator() as well as passing on to gui via getGrainDescription
    double _sample_index {0.0};
    float _waveform_read_rate {0.0};
    enum class FrequencyRandomizationMode {
        Continuous = 0,
        Octaves
    } _frequencyRandomizationMode;
    float _window_val {0.f};
	float _pan {0.f};
	float _grain_weight {1.f};

    float _random_pitch_ratio {1.f};
    float _ratio_based_on_note {1.f}; // =1.f. later this may change according to a settable concert pitch
    float _amplitude_based_on_note {0.f};

    float _grain_normalize_amount {0.f};    // works as lerp between no normalization to full normalization
	float _grain_makeup_gain {1.0f};

    bool _pitchify { false };
    float _duration_pitch_compensation_factor {1.f};
    double _duration_in_samps {441.0};
    float _underlying_f0 {0.f};

    struct GrainWindow {
        GrainWindow(const double duration, const float transpositionMultiplier, const float skew, const float plateau)
            : invDurationTransposition(1.0 / (duration * static_cast<double>(transpositionMultiplier)))
            , kp(std::abs(skew) + 1.f)
            , shapeExponent((plateau <= 0.f) ? 1.f - plateau : 1.f + plateau)
            , normCoef(g_refined(plateau))
            , skewPositive(skew >= 0.f)
            , usePlateauPath(plateau > 0.f)
            , warpIsIdentity(skew == 0.f)
            , shapeIsIdentity(plateau == 0.f)
        {
            assert(transpositionMultiplier > 0.f);
            assert(duration > 0.0);
            assert(normCoef > 0.f);
        }

        float calculate(const double accum) const {
            const float x = static_cast<float>(
                memoryless::clamp(accum * invDurationTransposition, 0.0, 1.0)
            );

            const float xWarped = [&]() -> float {
                const float base = skewPositive ? x : 1.f - x;
                if (base <= 0.f) return 0.f;
                if (base >= 1.f) return 1.f;
                return warpIsIdentity ? base : std::pow(base, kp);
            }();

            const float w = [&]() -> float {
                if (shapeIsIdentity) {
                    if (xWarped <= 0.f || xWarped >= 1.f) return 0.f;
                    return (xWarped <= 0.5f)
                        ? s(2.f * xWarped)
                        : 1.f - s(2.f * xWarped - 1.f);
                }
                return usePlateauPath ? fT(xWarped, shapeExponent) : sT(xWarped, shapeExponent);
            }();
            jassert (w >= 0.f); jassert (w <= 1.f);
            return w * normCoef;
        }

    private:
        double invDurationTransposition;
        float kp;
        float shapeExponent;
        float normCoef;
        bool skewPositive;
        bool usePlateauPath;
        bool warpIsIdentity;
        bool shapeIsIdentity;

        static float g(float plateau) {
            // rational approximation of integral of window (taking into account only plateau)
            // used np.linalg.lstsq to find coefficients.
            plateau = std::min(plateau, 25.f);
            const auto x = plateau;
            const auto x2 = x * x;
            const auto x3 = x * x2;
            // g(x) =
            //      (0.98929477  + -0.13804840x + 0.05481294x² + -0.00190264x³)
            //      -----------------------------------------------------------
            //             (1 + 0.09297753x + 0.06866064x² + 0.00112537x³)
            const auto num = 0.98929477 +
                            -0.13804840 * x +
                             0.05481294 * x2 +
                            -0.00190264 * x3;
            const auto den = 1.0 +
                             0.09297753 * x +
                             0.06866064 * x2 +
                             0.00112537 * x3;
            return num / den;
        }
        static float g_refined(float plateau) {
            // using g's coefficients as starting point, optimized further via scipy.optimize.least_squares
            plateau = std::min(plateau, 36.f);
            const auto x = plateau;
            const auto x2 = x * x;
            const auto x3 = x * x2;
            // g_ref(x) =
            //       (1.00018093 + -0.21485195x + 0.03825350x² + -0.00085922x³)
            //       ----------------------------------------------------------
            //            (1 + 0.00328264x + 0.03852408x² + 0.00000751x³)
            const auto num = 1.00018093 +
                -0.21485195 * x +
                 0.03825350 * x2 +
                -0.00085922 * x3;
            const auto den = 1.0 +
                 0.00328264 * x +
                 0.03852408 * x2 +
                 0.00000751 * x3;
            return num / den;
        }

        static float s(const float v) {
            if (v <= 0.f) return 0.f;
            if (v >= 1.f) return 1.f;
            return v * v * (3.f - 2.f * v);
        }

        static float sT(const float v, const float p) {
            if (v <= 0.f || v >= 1.f) return 0.f;
            return (v <= 0.5f)
                ? std::pow(s(2.f * v), p)
                : std::pow(1.f - s(2.f * v - 1.f), p);
        }

        static float fT(const float v, const float p) {
            if (v <= 0.f || v >= 1.f) return 0.f;
            return (v <= 0.5f)
                ? 1.f - std::pow(s(1.f - 2.f * v), p)
                : 1.f - std::pow(s(2.f * v - 1.f), p);
        }
    };
    GrainWindow _grainWindow;
};

} // namespace nvs::gran
