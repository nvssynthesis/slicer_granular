/*
  ==============================================================================

    AmpEnvelopePage.cpp
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "AmpEnvelopePage.h"
#include "../SlicerGranularPluginProcessor.h"
#include "../StringAxiom.h"
#include "../Synthesis/BreakpointEnvelopeShape.h"

namespace {

// converts a persisted (proportion-based) shape into the editor's fixed-pixel-width
// descriptor. the sustain segment gets an arbitrary fixed share of the width purely so
// it's visible/draggable in the editor -- it never affects real playback timing.
MultiSegmentEnvelopeGenerator::Descriptor toEditorDescriptor(const nvs::gran::BreakpointEnvShape &shape) {
	MultiSegmentEnvelopeGenerator::Descriptor desc;
	if (shape.segments.empty()) return desc;

	constexpr int totalWidth = EnvelopeEditor::MSEG_DEFAULT_PIXELS_WIDTH;
	constexpr int sustainWidth = totalWidth / 10;
	const int nonSustainWidth = totalWidth - sustainWidth;

	const int sustainIdx = jlimit(0, static_cast<int>(shape.segments.size()) - 1, shape.sustainIndex);

	desc.reserve(shape.segments.size());
	for (int i = 0; i < static_cast<int>(shape.segments.size()); ++i) {
		const auto &seg = shape.segments[i];
		MultiSegmentEnvelopeGenerator::SegmentDescriptor sd {};
		sd.initialValue = seg.initialValue;
		sd.finalValue = seg.finalValue;
		sd.curvature = seg.curvature;
		sd.lengthSamples = (i == sustainIdx)
			? sustainWidth
			: jmax(1, static_cast<int>(seg.proportion * static_cast<float>(nonSustainWidth)));
		desc.push_back(sd);
	}
	return desc;
}

// converts the editor's fixed-pixel-width descriptor back into proportions (normalized
// over the non-sustain segments), for persistence/publishing.
nvs::gran::BreakpointEnvShape toShape(const MultiSegmentEnvelopeGenerator::Descriptor &desc, int sustainIdx) {
	nvs::gran::BreakpointEnvShape shape;
	if (desc.empty()) return shape;

	sustainIdx = jlimit(0, static_cast<int>(desc.size()) - 1, sustainIdx);
	shape.sustainIndex = sustainIdx;

	double totalNonSustain = 0.0;
	for (int i = 0; i < static_cast<int>(desc.size()); ++i) {
		if (i == sustainIdx) continue;
		totalNonSustain += jmax(0, desc[i].lengthSamples);
	}
	if (totalNonSustain <= 0.0) totalNonSustain = 1.0;

	shape.segments.reserve(desc.size());
	for (int i = 0; i < static_cast<int>(desc.size()); ++i) {
		const auto &seg = desc[i];
		const float proportion = (i == sustainIdx)
			? 0.f
			: static_cast<float>(jmax(0, seg.lengthSamples) / totalNonSustain);
		shape.segments.push_back({ seg.initialValue, seg.finalValue, seg.curvature, proportion });
	}
	return shape;
}

}	// namespace

AmpEnvelopePage::AmpEnvelopePage(AudioProcessorValueTreeState &apvts, SlicerGranularAudioProcessor &processor)
:	_apvts(apvts)
,	_processor(processor)
,	_modeCombo(apvts, nvs::param::ParameterRegistry::getParameterByID("amp_env_mode"))
,	_lengthSlider(apvts, nvs::param::ParameterRegistry::getParameterByID("amp_env_bp_length"), Slider::SliderStyle::LinearBarVertical)
{
	using namespace nvs::param;
	for (auto const id : { "amp_env_attack", "amp_env_decay", "amp_env_sustain", "amp_env_release" }) {
		_adsrSliders.add(new AttachedSlider(apvts, ParameterRegistry::getParameterByID(id), Slider::SliderStyle::LinearBarVertical));
	}

	addAndMakeVisible(_modeCombo);
	for (auto *s : _adsrSliders) addAndMakeVisible(s);
	addAndMakeVisible(_lengthSlider);
	addAndMakeVisible(_envelopeEditor);

	// load whatever's persisted (SlicerGranularAudioProcessor guarantees this child exists,
	// with a sensible default, by the time the editor can be opened).
	if (const auto tree = apvts.state.getChildWithName(nvs::axiom::AmpBreakpointEnv); tree.isValid()) {
		const auto shape = nvs::gran::loadBreakpointEnvShapeFromValueTree(tree);
		if (const auto desc = toEditorDescriptor(shape); !desc.empty()) {
			_envelopeEditor.setDescriptor(desc, shape.sustainIndex);
		}
	}

	_envelopeEditor.addChangeListener(this);
	apvts.addParameterListener("amp_env_mode", this);

	setBreakpointVisible(static_cast<int>(*apvts.getRawParameterValue("amp_env_mode")) != 0);
}

AmpEnvelopePage::~AmpEnvelopePage() {
	_apvts.removeParameterListener("amp_env_mode", this);
	_envelopeEditor.removeChangeListener(this);
}

void AmpEnvelopePage::resized() {
	auto r = getLocalBounds();
	constexpr int comboHeight = 24;
	_modeCombo.setBounds(r.removeFromTop(comboHeight).reduced(4, 0));
	r.removeFromTop(4);

	{
		const int n = _adsrSliders.size();
		if (n > 0) {
			const int w = r.getWidth() / n;
			int x = r.getX();
			for (auto *s : _adsrSliders) {
				s->setBounds(x, r.getY(), w, r.getHeight());
				x += w;
			}
		}
	}
	{
		auto bpBounds = r;
		constexpr int lengthWidth = 60;
		_lengthSlider.setBounds(bpBounds.removeFromLeft(lengthWidth));
		_envelopeEditor.setBounds(bpBounds);
	}
}

void AmpEnvelopePage::parameterChanged(const String &, float newValue) {
	const bool useBreakpoint = static_cast<int>(newValue) != 0;
	Component::SafePointer<AmpEnvelopePage> safeThis(this);
	MessageManager::callAsync([safeThis, useBreakpoint] {
		if (auto *self = safeThis.getComponent()) self->setBreakpointVisible(useBreakpoint);
	});
}

void AmpEnvelopePage::changeListenerCallback(ChangeBroadcaster *source) {
	if (source == &_envelopeEditor) {
		publishAndPersist();
	}
}

void AmpEnvelopePage::setBreakpointVisible(const bool useBreakpoint) {
	for (auto *s : _adsrSliders) s->setVisible(!useBreakpoint);
	_lengthSlider.setVisible(useBreakpoint);
	_envelopeEditor.setVisible(useBreakpoint);
}

void AmpEnvelopePage::publishAndPersist() const {
	const auto shape = toShape(_envelopeEditor.getDescriptor(), _envelopeEditor.getSustainSegmentIndex());

	auto tree = _apvts.state.getOrCreateChildWithName(nvs::axiom::AmpBreakpointEnv, nullptr);
	nvs::gran::saveBreakpointEnvShapeToValueTree(tree, shape);
	_processor.publishAmpBreakpointEnvShape(shape);
}
