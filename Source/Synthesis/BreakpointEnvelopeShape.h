/*
  ==============================================================================

    BreakpointEnvelopeShape.h

    Plugin-side data model for the breakpoint amp envelope: a persisted,
    variable-length list of segments (proportions of a total length, plus one
    designated "sustain" segment) and the conversion into the fork's runtime
    MultiSegmentEnvelopeGenerator::Descriptor (absolute sample counts).

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
	float proportion {0.f};	// proportion of total non-sustain length; ignored for the sustain segment
};

struct BreakpointEnvShape {
	std::vector<BreakpointSegment> segments;
	int sustainIndex {0};
};

// mirrors the current ADSR shape (attack, decay-to-sustain, an explicit flat sustain
// segment, release) so switching into breakpoint mode for the first time sounds the same
// as the ADSR did, before any editing. attack/decay/release are only used for their
// relative proportions -- the absolute length comes from the separate "Length" parameter.
inline BreakpointEnvShape defaultAdsrMirrorShape(float attackSeconds, float decaySeconds,
												  float sustainLevel, float releaseSeconds) {
	const float total = juce::jmax(attackSeconds + decaySeconds + releaseSeconds, 1e-6f);

	BreakpointEnvShape shape;
	shape.segments = {
		{ 0.f,			1.f,			0.f, attackSeconds / total },
		{ 1.f,			sustainLevel,	0.f, decaySeconds  / total },
		{ sustainLevel,	sustainLevel,	0.f, 0.f },	// sustain: proportion unused, held indefinitely
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

// converts proportions -> absolute sample counts. the sustain segment always becomes an
// infinite hold (negative lengthSamples, flat value) regardless of its stored proportion,
// leveraging SegmentGenerator::getSample's existing "non-timed sustain segment" behavior.
inline MultiSegmentEnvelopeGenerator::Descriptor toRuntimeDescriptor(const BreakpointEnvShape &shape,
																	  double lengthSeconds, double sampleRate) {
	MultiSegmentEnvelopeGenerator::Descriptor desc;
	if (shape.segments.empty()) return desc;

	const int sustainIdx = juce::jlimit(0, static_cast<int>(shape.segments.size()) - 1, shape.sustainIndex);

	float proportionSum = 0.f;
	for (int i = 0; i < static_cast<int>(shape.segments.size()); ++i) {
		if (i == sustainIdx) continue;
		proportionSum += juce::jmax(0.f, shape.segments[i].proportion);
	}
	if (proportionSum <= 0.f) proportionSum = 1.f;

	desc.reserve(shape.segments.size());
	for (int i = 0; i < static_cast<int>(shape.segments.size()); ++i) {
		const auto &seg = shape.segments[i];
		MultiSegmentEnvelopeGenerator::SegmentDescriptor rt {};
		rt.curvature = seg.curvature;

		if (i == sustainIdx) {
			rt.initialValue = seg.initialValue;
			rt.finalValue = seg.initialValue;	// forced flat: required for the infinite-hold path
			rt.lengthSamples = -1;
		} else {
			rt.initialValue = seg.initialValue;
			rt.finalValue = seg.finalValue;
			const double normalizedProportion = juce::jmax(0.f, seg.proportion) / proportionSum;
			const double segSeconds = normalizedProportion * lengthSeconds;
			rt.lengthSamples = juce::jmax(1, static_cast<int>(segSeconds * sampleRate));
		}
		desc.push_back(rt);
	}
	return desc;
}

}	// namespace nvs::gran
