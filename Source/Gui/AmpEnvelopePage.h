/*
  ==============================================================================

    AmpEnvelopePage.h

    "Envelope" tab: a mode toggle (amp_env_mode) between the plain ADSR sliders
    and an embedded breakpoint-envelope editor (juce-MultiStepEnvelopeGenerator
    fork). Visibility swaps immediately on mode change; the breakpoint shape is
    persisted into the apvts state tree and published to the audio engine on
    every edit (see BreakpointEnvelopeShape.h / GranularVoice).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "AttachedSlider.h"
#include "AttachedComboBox.h"
#include "../Params/params.h"
#include "EnvelopeEditor.h"

class SlicerGranularAudioProcessor;

struct AmpEnvelopePage final	:	public Component
,								private AudioProcessorValueTreeState::Listener
,								private ChangeListener
{
	AmpEnvelopePage(AudioProcessorValueTreeState &apvts, SlicerGranularAudioProcessor &processor);
	~AmpEnvelopePage() override;

	void resized() override;

private:
	void parameterChanged(const String &parameterID, float newValue) override;
	void changeListenerCallback(ChangeBroadcaster *source) override;
	void setBreakpointVisible(bool useBreakpoint);
	void publishAndPersist() const;

	AudioProcessorValueTreeState &_apvts;
	SlicerGranularAudioProcessor &_processor;

	AttachedComboBox _modeCombo;
	OwnedArray<AttachedSlider> _adsrSliders;
	AttachedSlider _lengthSlider;
	EnvelopeEditor _envelopeEditor;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpEnvelopePage)
};
