/*
  ==============================================================================

    BreakpointEnvelopeShape.h

    Plugin-side data model for the breakpoint amp envelope: a persisted,
    variable-length list of segments (proportions of a total length) plus a
    designated "sustain point" -- the boundary between two segments at which
    playback freezes while a note is held -- and the conversion into the
    fork's runtime MultiSegmentEnvelopeGenerator::Descriptor (absolute sample
    counts).

    Sustain is a *point*, not a segment: segments [0, sustainIndex) play once
    as the attack/decay portion, then playback freezes at that boundary value
    for as long as the note is held (see GranularVoice, which implements the
    freeze -- the fork's generator has no concept of this). On note-off,
    playback resumes from there through segments [sustainIndex, N) as the
    release. sustainIndex == 0 means "freeze immediately at note-on, play the
    whole shape as the release on note-off"; sustainIndex == N-1 means "only
    the last segment plays as the release."

    Kept apvts-agnostic: callers own reading/writing apvts state and pass in
    plain values.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SegmentGenerator.h"
#include "StringAxiom.h"

namespace nvs::gran {

struct BreakpointSegment {
	float initialValue {0.f};
	float finalValue {0.f};
	float curvature {0.f};
	float proportion {0.f};	// proportion of total shape length
};

struct BreakpointEnvShape {
	std::vector<BreakpointSegment> segments;
	int sustainIndex {0};	// index of the first segment played after note-off
};

// mirrors the current ADSR shape (attack, decay-to-sustain, release) so switching into
// breakpoint mode for the first time sounds the same as the ADSR did, before any editing.
// the sustain point sits right after the decay segment, at sustainLevel. attack/decay/release
// are only used for their relative proportions -- the absolute length comes from the
// separate "Length" parameter.
inline BreakpointEnvShape defaultAdsrMirrorShape(float attackSeconds, float decaySeconds,
												  float sustainLevel, float releaseSeconds) {
	const float total = juce::jmax(attackSeconds + decaySeconds + releaseSeconds, 1e-6f);

	BreakpointEnvShape shape;
	shape.segments = {
		{ 0.f,			1.f,			0.f, attackSeconds / total },
		{ 1.f,			sustainLevel,	0.f, decaySeconds  / total },
		{ sustainLevel,	0.f,			0.f, releaseSeconds / total },
	};
	shape.sustainIndex = 2;
	return shape;
}

inline BreakpointEnvShape loadBreakpointEnvShapeFromValueTree(const juce::ValueTree &tree) {
	BreakpointEnvShape shape;
	if (!tree.isValid()) return shape;

	shape.sustainIndex = tree.getProperty(nvs::axiom::bpSustainIndex, 0);
	for (int i = 0; i < tree.getNumChildren(); ++i) {
		const auto child = tree.getChild(i);
		shape.segments.push_back({
			static_cast<float>(child.getProperty(nvs::axiom::bpInitialValue, 0.f)),
			static_cast<float>(child.getProperty(nvs::axiom::bpFinalValue, 0.f)),
			static_cast<float>(child.getProperty(nvs::axiom::bpCurvature, 0.f)),
			static_cast<float>(child.getProperty(nvs::axiom::bpProportion, 0.f))
		});
	}
	return shape;
}

inline void saveBreakpointEnvShapeToValueTree(juce::ValueTree &tree, const BreakpointEnvShape &shape) {
	tree.removeAllChildren(nullptr);
	tree.setProperty(nvs::axiom::bpSustainIndex, shape.sustainIndex, nullptr);
	for (auto const &seg : shape.segments) {
		juce::ValueTree child(nvs::axiom::bpSegment);
		child.setProperty(nvs::axiom::bpInitialValue, seg.initialValue, nullptr);
		child.setProperty(nvs::axiom::bpFinalValue, seg.finalValue, nullptr);
		child.setProperty(nvs::axiom::bpCurvature, seg.curvature, nullptr);
		child.setProperty(nvs::axiom::bpProportion, seg.proportion, nullptr);
		tree.appendChild(child, nullptr);
	}
}

// converts proportions -> absolute sample counts. every segment plays for a real, positive
// length; the sustain point is not represented in the descriptor at all -- it's purely an
// index that GranularVoice uses to decide when to freeze/resume (see file header comment).
inline MultiSegmentEnvelopeGenerator::Descriptor toRuntimeDescriptor(const BreakpointEnvShape &shape,
																	  double lengthSeconds, double sampleRate) {
	MultiSegmentEnvelopeGenerator::Descriptor desc;
	if (shape.segments.empty()) return desc;

	float proportionSum = 0.f;
	for (auto const &seg : shape.segments) {
		proportionSum += juce::jmax(0.f, seg.proportion);
	}
	if (proportionSum <= 0.f) proportionSum = 1.f;

	desc.reserve(shape.segments.size());
	for (auto const &seg : shape.segments) {
		MultiSegmentEnvelopeGenerator::SegmentDescriptor rt {};
		rt.curvature = seg.curvature;
		rt.initialValue = seg.initialValue;
		rt.finalValue = seg.finalValue;
		const double normalizedProportion = juce::jmax(0.f, seg.proportion) / proportionSum;
		const double segSeconds = normalizedProportion * lengthSeconds;
		rt.lengthSamples = juce::jmax(1, static_cast<int>(segSeconds * sampleRate));
		desc.push_back(rt);
	}
	return desc;
}

}	// namespace nvs::gran
