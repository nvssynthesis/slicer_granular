/*
  ==============================================================================

    JuceGranularSynth.h
    Created: 22 Apr 2024 1:14:38am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "./JuceGranularSynthVoice.h"
#include <JuceHeader.h>

class GranularSynthesizer	:	public juce::Synthesiser
{
public:
	GranularSynthesizer();
	
	void setAudioBlock(juce::AudioBuffer<float> &waveBuffer, double newFileSampleRate);
	static constexpr int getNumVoices(){ return num_voices; }
	std::vector<nvs::gran::GrainDescription> getGrainDescriptions() const;
	void setCurrentPlaybackSampleRate(double newSampleRate) override;
	
	template <auto Start, auto End>
	constexpr void granularMainParamSet(juce::AudioProcessorValueTreeState &apvts){
		if constexpr (Start < End){
			juce::SynthesiserVoice* voice = getVoice(Start);
			if (GranularVoice* granularVoice = dynamic_cast<GranularVoice*>(voice)){
				granularVoice->granularMainParamSet<0, static_cast<int>(params_e::count_main_granular_params)>(apvts);
			}
			else {
				jassert (false);
			}
			granularMainParamSet<Start + 1, End>(apvts);
		}
	}
	
	template <auto Start, auto End>
	constexpr void envelopeParamSet(juce::AudioProcessorValueTreeState &apvts){
		if constexpr (Start < End){
			juce::SynthesiserVoice* voice = getVoice(Start);
			if (GranularVoice* granularVoice = dynamic_cast<GranularVoice*>(voice)){
				granularVoice->envelopeParamSet<static_cast<int>(params_e::count_main_granular_params)+1,
												static_cast<int>(params_e::count_envelope_params)>
												(apvts);
			}
			else {
				jassert (false);
			}
			envelopeParamSet<Start + 1, End>(apvts);
		}
	}
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
	bool hasLogger() const {
		return _synth_shared_state._logger_func != nullptr;
	}
protected: constexpr static int num_voices =
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
											2;
#else
											8;
#endif
private:
	nvs::gran::GranularSynthSharedState _synth_shared_state;
	
	void initializeVoices();
	
	size_t totalNumGrains_;
	
//==============================================================================================================
	void writeToLog(const juce::String &s){
		_synth_shared_state._logger_func(prepend_msg);
		_synth_shared_state._logger_func(s);
	}
	juce::String prepend_msg {"GranularSynthesizer: "};
};
