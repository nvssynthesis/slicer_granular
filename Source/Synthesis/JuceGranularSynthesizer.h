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
	GranularSynthesizer(double const &sampleRate,
						juce::AudioBuffer<float> &waveBuffer, double const &fileSampleRate,
						unsigned int num_voices);
	void setAudioBlock(juce::AudioBuffer<float> &waveBuffer);
	
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
private:
	std::function<void(const juce::String&)> logger = nullptr;
};
