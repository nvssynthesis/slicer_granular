/*
  ==============================================================================

    JuceGranularSynth.h
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "./GranularVoice.h"

namespace nvs::gran {
class GranularSynthesizer
#ifndef TSN
                            final
#endif
    :	public juce::Synthesiser
{
public:
    explicit GranularSynthesizer(juce::AudioProcessorValueTreeState &apvts);
    void setAudioBuffer(juce::AudioBuffer<float> &waveBuffer, double newFileSampleRate, juce::int64 audioHash);

    // sizes the shared reverb-send buss and prepares the single shared reverb; must be called
    // before rendering, and again whenever sampleRate/samplesPerBlock change.
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    virtual void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
    {
        // wrapper around renderNextBlock that should also manage any synth-global necessities (because otherwise we must take care
        // of DSP voicewise-only, since renderNextBlock is not virtual and it accumulates samples voicewise)
        renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    }
    static constexpr int getNumVoices(){ return num_voices; }
    std::vector<GrainDescription> getGrainDescriptions() const;
    void setCurrentPlaybackSampleRate(double newSampleRate) override;

    enum class PositionAlignmentSetting {
        alignAtWindowStart = 0,
        alignAtWindowPeak = 1
    };
    void setPositionAlignmentSetting(PositionAlignmentSetting setting) {
        _synth_shared_state._settings._center_position_at_env_peak = static_cast<bool>(setting);
    }

    void setLogger(std::function<void(const juce::String&)> loggerFunction);
    bool hasLogger() const {
        return _synth_shared_state._logger_func != nullptr;
    }
    GranularSynthSharedState const &viewSynthSharedState() {
        return _synth_shared_state;
    }

    // called from the GUI thread on every breakpoint-envelope edit.
    void publishAmpBreakpointEnvShape(const BreakpointEnvShape &shape) {
        const juce::SpinLock::ScopedLockType lock(_synth_shared_state._amp_breakpoint_env_lock);
        _synth_shared_state._amp_breakpoint_env_shape = shape;
    }
protected:
    constexpr static int num_voices = N_VOICES;
    GranularSynthSharedState _synth_shared_state;

    // called by juce::Synthesiser once per (MIDI-accurate) sub-block of actual voice rendering.
    // wraps the base voice-rendering pass with the single shared reverb: grains have already
    // written their per-grain sends into _synth_shared_state._reverb_send_buffer by the time
    // the base call returns, so the reverb reads/processes exactly that range and gets mixed
    // into outputAudio alongside the (already-written) dry voice output.
    void renderVoices(juce::AudioBuffer<float> &outputAudio, int startSample, int numSamples) override;

    juce::Reverb _reverb;
private:
    void initializeVoices();
    size_t totalNumGrains_;
    //==============================================================================================================
    void writeToLog(const juce::String &s){
        _synth_shared_state._logger_func(prepend_msg);
        _synth_shared_state._logger_func(s);
    }
    juce::String prepend_msg {"GranularSynthesizer: "};
};
}   // namespace nvs::gran