/*
  ==============================================================================

    JuceGranularSynthVoice.h
    Created: 7 May 2024 11:04:44am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "./GranularSynthesis.h"
#include "../Params/params.h"
#include "../Params/GranularParameterMapping.h"


class GranularVoice	:	public juce::SynthesiserVoice
{
public:
	GranularVoice(std::unique_ptr<nvs::gran::PolyGrain> synthGuts)
	:	granularSynthGuts{std::move(synthGuts)}
	{
		granularSynthGuts->setReadBounds({0.0, 1.0});
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
	
	void setAmpAttack(float newAttack);
	void setAmpDecay(float newDecay);
	void setAmpSustain(float newSustain);
	void setAmpRelease(float newRelease);
	
	nvs::gran::PolyGrain* getGranularSynthGuts(){
		return granularSynthGuts.get();
	}
	static size_t getNumGrains(){
		return nvs::gran::PolyGrain::getNumGrains();
	}
	void setLogger(std::function<void(const juce::String&)> loggerFunction);
private:
	std::unique_ptr<nvs::gran::PolyGrain> granularSynthGuts;
	nvs::gran::GranularVoiceSharedState _voice_shared_state;
	
	int lastMidiNoteNumber {0};
	std::vector<nvs::gran::GrainDescription> _grainDescriptions;
	juce::ADSR adsr;
	juce::ADSR::Parameters adsrParameters {0.1, 0.3, 0.5, 0.05};
	
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
