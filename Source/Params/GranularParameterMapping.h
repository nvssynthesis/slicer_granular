/*
  ==============================================================================

    GranularParameterMapping.h
    Created: 27 May 2025 12:48:25pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "params.h"
#include "../Synthesis/GranularSynthesis.h"

namespace nvs::param {

using Setter = void (nvs::gran::PolyGrain::*)(float);

static const std::unordered_map<juce::String,Setter> setterMap = {
	{ "transpose",     	&nvs::gran::PolyGrain::setTranspose    },
	{ "position",      	&nvs::gran::PolyGrain::setPosition     },
	{ "fx_grain_drive",	&nvs::gran::PolyGrain::setFxGrainDrive },
	{ "fx_makeup_gain",	&nvs::gran::PolyGrain::setFxMakeupGain }
};

inline void updateParameter(nvs::gran::PolyGrain* target,
					 juce::StringRef paramID,
					 float value)
{
	auto it = setterMap.find(paramID);
	if (it != setterMap.end())
	{
		(target->*(it->second))(value);
	}
	else
	{
		jassertfalse;
	}
}

}	// nvs::param
