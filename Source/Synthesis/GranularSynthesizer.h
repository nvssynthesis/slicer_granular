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
    void setAudioBuffer(juce::AudioBuffer<float> &waveBuffer, double newFileSampleRate, size_t fileNameHash);

    virtual void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
    {
        // wrapper around renderNextBlock that should also manage any synth-global necessities (because otherwise we must take care
        // of DSP voicewise-only, since renderNextBlock is not virtual and it accumulates samples voicewise)
        renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    }
    static constexpr int getNumVoices(){ return num_voices; }
    std::vector<nvs::gran::GrainDescription> getGrainDescriptions() const;
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
    nvs::gran::GranularSynthSharedState const &viewSynthSharedState() {
        return _synth_shared_state;
    }
protected:
    constexpr static int num_voices = N_VOICES;
    nvs::gran::GranularSynthSharedState _synth_shared_state;
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