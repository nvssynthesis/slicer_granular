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
#include "../Params/params.h"

namespace nvs::gran {
template<typename T>
concept GranularSynthGuts = std::derived_from<T, nvs::gran::PolyGrain>;

class GranularVoice	:	public juce::SynthesiserVoice
{
public:
    template<GranularSynthGuts GranularSynthGuts_t>
    static GranularVoice *create(nvs::gran::GranularSynthSharedState *const synth_shared_state, unsigned long seed, int voice_id) {
        auto* voice = new GranularVoice(synth_shared_state, seed, voice_id);
        voice->initSynthGuts<GranularSynthGuts_t>();
        return voice;
    }

    void setCurrentPlaybackSampleRate(double sampleRate) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);	// why not override??

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    bool isVoiceActive() const override;

    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    bool canPlaySound (juce::SynthesiserSound *) override ;

    std::vector<nvs::gran::GrainDescription> getGrainDescriptions() const;

    nvs::gran::PolyGrain* getGranularSynthGuts(){
        return granularSynthGuts.get();
    }
    static size_t getNumGrains(){
        return nvs::gran::PolyGrain::getNumGrains();
    }
    void setLogger(std::function<void(const juce::String&)> loggerFunction);
private:
    GranularVoice(nvs::gran::GranularSynthSharedState *const synth_shared_state, unsigned long seed, int voice_id);

    template <GranularSynthGuts GranularSynthGuts_t>
    void initSynthGuts();

    nvs::gran::GranularSynthSharedState *_synth_shared_state {nullptr};
    nvs::gran::GranularVoiceSharedState _voice_shared_state;
    std::unique_ptr<nvs::gran::PolyGrain> granularSynthGuts;

    int lastMidiNoteNumber {0};
    std::vector<nvs::gran::GrainDescription> _grainDescriptions;
    juce::ADSR adsr;

    std::function<void(const juce::String&)> logger = nullptr;

    struct dbg_counter {
        using int_t = unsigned long;
        int_t i {0};
        bool go(int_t cmp){
            if (++i == cmp){
                i = 0;
                return true;
            } else {
                return false;
            }
        }
    } counter;
};


template<>
inline void GranularVoice::initSynthGuts<nvs::gran::PolyGrain>() {
    granularSynthGuts = std::make_unique<nvs::gran::PolyGrain>(_synth_shared_state, &_voice_shared_state);
    granularSynthGuts->setReadBounds({0.0, 1.0});
}

}   // namespace nvs::gran