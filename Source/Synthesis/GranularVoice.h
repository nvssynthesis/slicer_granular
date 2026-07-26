/*
  ==============================================================================

	GranularVoice.h
    Created: 7 May 2024 11:04:44am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "./GranularSynthesis.h"
#include "./BreakpointEnvelopeShape.h"
#include "../Params/params.h"

namespace nvs::gran {
template<typename T>
concept GranularSynthGuts = std::derived_from<T, PolyGrain>;

class GranularVoice	:	public SynthesiserVoice
{
public:
    template<GranularSynthGuts GranularSynthGuts_t>
    static GranularVoice *create(GranularSynthSharedState *const synth_shared_state, const unsigned long seed, const int voice_id) {
        auto* voice = new GranularVoice(synth_shared_state, seed, voice_id);
        voice->initSynthGuts<GranularSynthGuts_t>();
        return voice;
    }

    void setCurrentPlaybackSampleRate(double sampleRate) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);	// why not override??

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound *sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    bool isVoiceActive() const override;

    void renderNextBlock (AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    bool canPlaySound (SynthesiserSound *) override ;

    std::vector<GrainDescription> getGrainDescriptions() const;

    PolyGrain* getGranularSynthGuts(){
        return granularSynthGuts.get();
    }
    static size_t getNumGrains(){
        return PolyGrain::getNumGrains();
    }
    void setLogger(const std::function<void(const String &)> &loggerFunction);
private:
    GranularVoice(GranularSynthSharedState *synth_shared_state, unsigned long seed, int voice_id);

    template <GranularSynthGuts GranularSynthGuts_t>
    void initSynthGuts();

    GranularSynthSharedState *_synth_shared_state {nullptr};
    GranularVoiceSharedState _voice_shared_state;
    std::unique_ptr<PolyGrain> granularSynthGuts;

    int lastMidiNoteNumber {0};
    std::vector<GrainDescription> _grainDescriptions;
    ADSR adsr;

    // alternative amp envelope engine (see BreakpointEnvelopeShape.h). the mode is decided
    // once at startNote and held for the note's whole lifetime, so switching modes mid-note
    // never changes what's already sounding.
    MultiSegmentEnvelopeGenerator breakpointEnv {512};
    MultiSegmentEnvelopeGenerator::Descriptor breakpointRuntimeDesc;	// owned storage; generator only holds a raw pointer to it
    BreakpointEnvShape cachedBreakpointShape;	// last shape seen; fallback if the try-lock at startNote fails
    bool noteUsesBreakpointEnv {false};
    int sustainSegIndexForThisNote {0};
    bool breakpointReleaseTriggered {false};
    bool breakpointEnvActive {false};

    std::function<void(const String&)> logger = nullptr;

    struct dbg_counter {
        using int_t = unsigned long;
        int_t i {0};
        bool go(int_t cmp) {
            if (++i == cmp){
                i = 0;
                return true;
            }
            return false;
        }
    } counter;
};


template<>
inline void GranularVoice::initSynthGuts<PolyGrain>() {
    granularSynthGuts = std::make_unique<PolyGrain>(_synth_shared_state, &_voice_shared_state);
    granularSynthGuts->setReadBounds({0.0, 1.0});
}

}   // namespace nvs::gran