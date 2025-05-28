/*
  ==============================================================================

    params.h
    Created: 16 Jun 2023 5:37:13pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <map>
#include <tuple>
#include <JuceHeader.h>

/*** TODO:
 -octave
 -fine tuning
 -add automatic traversal
	-(really this could just be an LFO => Position. Then, it can easily be
	routed anywhere just like randomness).
 -master volume
 */
namespace nvs::param {

struct ParameterDef {
	juce::String ID;
	juce::String displayName;  // for UI display (can differ from internal name)
	juce::String groupName;
	
	// Core range values
	float min, max, defaultVal;
	float interval = 0.0f;  // step size (0 = continuous)
	float skew = 1.0f;
	bool symmetrical = false;
	
	// Advanced range functions (optional)
	std::function<float(float,float,float)> convertFrom0To1 = nullptr;
	std::function<float(float,float,float)> convertTo0To1 = nullptr;
	std::function<float(float,float,float)> snapToLegalValue = nullptr;
	
	// UI metadata only - no implementation coupling
	juce::String unitSuffix = "";  // e.g., "dB", "Hz", "%"
	int numDecimalPlaces = 2;
	
	// convenience constructors for common parameter types
	static ParameterDef linear(juce::StringRef name, juce::StringRef displayName,
							   float min, float max, float defaultVal,
							   juce::StringRef groupName,
							   juce::StringRef unitSuffix = "", float interval = 0.0f);
	
	static ParameterDef skewed(juce::StringRef name, juce::StringRef displayName,
									float min, float max, float defaultVal,
									juce::StringRef groupName,
									juce::StringRef unitSuffix = "",
									float skew=0.3f, bool useSymmetricSkew=false);
	
	static ParameterDef decibel(juce::StringRef name, juce::StringRef displayName,
								float minDB, float maxDB, float defaultDB,
								juce::StringRef groupName);
	
	template<typename T>
	juce::NormalisableRange<T> createNormalisableRange() const {
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
					  "T must be float or double");
		
		if (convertFrom0To1 && convertTo0To1) {
			// Capture the std::function objects by value, not 'this'
			auto convertFrom0To1Copy = convertFrom0To1;
			auto convertTo0To1Copy = convertTo0To1;
			
			auto from0To1 = [convertFrom0To1Copy](T start, T end, T val) -> T {
				return static_cast<T>(convertFrom0To1Copy(float(start), float(end), float(val)));
			};
			auto to0To1 = [convertTo0To1Copy](T start, T end, T val) -> T {
				return static_cast<T>(convertTo0To1Copy(float(start), float(end), float(val)));
			};
			
			std::function<T(T,T,T)> snapFunc = nullptr;
			if (snapToLegalValue) {
				auto snapToLegalValueCopy = snapToLegalValue;
				snapFunc = [snapToLegalValueCopy](T start, T end, T val) -> T {
					return static_cast<T>(snapToLegalValueCopy(float(start), float(end), float(val)));
				};
			}
			
			return juce::NormalisableRange<T>(static_cast<T>(min), static_cast<T>(max),
											  from0To1, to0To1, snapFunc);
		} else {
			// Standard constructor
			return juce::NormalisableRange<T>(static_cast<T>(min), static_cast<T>(max),
											  static_cast<T>(interval), static_cast<T>(skew),
											  symmetrical);
		}
	}
	
	// convenience methods for JUCE components
	juce::NormalisableRange<float> getFloatRange() const { return createNormalisableRange<float>(); }
	juce::NormalisableRange<double> getDoubleRange() const { return createNormalisableRange<double>(); }
};

// inline implementations of static factory methods
inline ParameterDef ParameterDef::linear(juce::StringRef name, juce::StringRef displayName,
										float min, float max, float defaultVal,
										juce::StringRef groupName,
										juce::StringRef unitSuffix, float interval) {
	return {name, displayName, groupName, min, max, defaultVal, interval, 1.0f, false,
		nullptr, nullptr, nullptr, unitSuffix, 2};
}

inline ParameterDef ParameterDef::skewed(juce::StringRef name, juce::StringRef displayName,
											float min, float max, float defaultVal,
											juce::StringRef groupName,
											juce::StringRef unitSuffix,
											float skew, bool symmetricSkew) {
	return {name, displayName, groupName, min, max, defaultVal, 0.0f, skew, false,
		nullptr, nullptr, nullptr, unitSuffix, 2};
}

inline ParameterDef ParameterDef::decibel (juce::StringRef ID,
										juce::StringRef displayName,
										float minDB, float maxDB, float defaultDB,
										juce::StringRef groupName)
{
	ParameterDef param;
	param.ID 				= ID;
	param.displayName 		= displayName;
	param.groupName 		= groupName;
	param.min 				= minDB;
	param.max 				= maxDB;
	param.defaultVal 		= defaultDB;
	param.unitSuffix 		= " dB";
	param.numDecimalPlaces 	= 1;
	
	// build linear-gain endpoints, clamped so vals <= minDB become 0.0f
	const float minGainCapture = juce::Decibels::decibelsToGain(minDB, minDB);
	const float maxGainCapture = juce::Decibels::decibelsToGain(maxDB, minDB);
	const float minDBCapture = minDB;
	
	// normalised [0..1] -> dB (via gain), with floor at minDB
	param.convertFrom0To1 = [minGainCapture, maxGainCapture, minDBCapture](float, float, float normVal) -> float
	{
		float gain = minGainCapture + normVal * (maxGainCapture - minGainCapture);
		return juce::Decibels::gainToDecibels(gain, minDBCapture);
	};
	
	// dB -> normalised [0..1], with any input <= minDB mapping to 0.0
	param.convertTo0To1 = [minGainCapture, maxGainCapture, minDBCapture](float, float, float dbVal) -> float
	{
		float gain = juce::Decibels::decibelsToGain(dbVal, minDBCapture);
		return (gain - minGainCapture) / (maxGainCapture - minGainCapture);
	};
	
	return param;
}

static constexpr float envTimingMin {0.01f};
static constexpr float envTimingMax {8.f};
static constexpr float envTimingSkew {0.5f};

// Clean parameter definitions - no implementation dependencies!
inline const std::vector<ParameterDef> ALL_PARAMETERS = {
	ParameterDef::linear("transpose", 	"Transpose", 	-60.f,	 60.f,		0.f, 	"Main", " semi"),
	ParameterDef::linear("position", 	"Position", 	0.f, 	1.f, 		0.f, 	"Main",	 "%"),
	ParameterDef::skewed("speed", "Speed", 				0.1f, 	1000.f, 	50.f, 	"Main", "hz"),
	ParameterDef::skewed("duration", "Duration", 		1e-6f, 	1.f, 		0.1f, 	"Main", "%"),
	ParameterDef::linear("skew", 	"Skew", 			5e-3f, 	1.f-5e-3f, 	0.5f, 	"Main",	 "%"),
	ParameterDef::skewed("plateau", "Plateau", 			0.01f, 	10.f, 		1.f, 	"Main",	 ""),
	ParameterDef::linear("pan", 	"Pan", 				0.f, 	1.f, 		0.5f, 	"Main",	 "%"),

	ParameterDef::skewed("transpose_rand", "Transpose Randomness", 		0.f, 1.f, 0.f, "MainRandom"),
	ParameterDef::skewed("position_rand", "Position Randomness", 		0.f, 1.f, 0.f, "MainRandom"),
	ParameterDef::skewed("speed_rand", "Speed Randomness", 				0.f, 1.f, 0.f, "MainRandom"),
	ParameterDef::skewed("duration_rand", "Duration Randomness", 		0.f, 1.f, 0.f, "MainRandom"),
	ParameterDef::skewed("skew_rand", "Skew Randomness",	 			0.f, 1.f, 0.f, "MainRandom"),
	ParameterDef::skewed("plateau_rand", "Plateau Randomness", 			0.f, 1.f, 0.f, "MainRandom"),
	ParameterDef::skewed("pan_rand", "Pan Randomness",		 			0.f, 1.f, 0.f, "MainRandom"),

	ParameterDef::skewed("amp_env_attack", 	"Attack", 	envTimingMin, 	envTimingMax, 	0.05f, 	"Amplitude Envelope", "Seconds"),
	ParameterDef::skewed("amp_env_decay", 	"Decay", 	envTimingMin, 	envTimingMax, 	1.0f, 	"Amplitude Envelope", "Seconds"),
	ParameterDef::skewed("amp_env_sustain", "Sustain", 	0.f, 			1.f, 			0.85f, 	"Amplitude Envelope", "%"),
	ParameterDef::skewed("amp_env_release", "Release", 	envTimingMin, 	envTimingMax, 	1.0f, 	"Amplitude Envelope", "Seconds"),

	ParameterDef::skewed("scanner_rate",	"Rate",		-20.f,			20.f,			0.f,	"Scanner", 	"Hz", 0.3f, true),
	ParameterDef::linear("scanner_amount",	"Amount",	0.f,			1.f,			0.f,	"Scanner", 	"%"),
	
#ifdef TSN
	// create TSN navigation and other params here
#endif
	
	ParameterDef::decibel("fx_grain_drive", "Grain Drive", -10.f, 40.f, 0.f,	"Fx"),
	ParameterDef::decibel("fx_makeup_gain", "Makeup Gain", -40.f, 20.f, 0.f, 	"Fx"),
};


struct ParameterRegistry {
public:
	static std::vector<ParameterDef> getParametersForGroup(juce::StringRef groupName);
	static const ParameterDef& getParameterByName(juce::StringRef name);
	static juce::StringArray getAllParameterNames();
	static size_t getParameterIndex(juce::StringRef name);
};
inline std::vector<ParameterDef> ParameterRegistry::getParametersForGroup(juce::StringRef groupName){
	std::vector<ParameterDef> params;
	for (auto const &pd : ALL_PARAMETERS){
		if (pd.groupName.equalsIgnoreCase(groupName)){
			params.push_back(pd);
		}
	}
	return params;
}
inline const ParameterDef& getParameterByID(juce::StringRef name) {
	auto it = std::find_if(ALL_PARAMETERS.begin(), ALL_PARAMETERS.end(),
	[name](ParameterDef const &pd){
		return pd.ID.equalsIgnoreCase(name);
	});
	if (it != ALL_PARAMETERS.end()){
		return *it;
	}
	jassertfalse;
	return ALL_PARAMETERS[0];	// just to avoid warning about not returning for all control paths
}
inline const size_t getParameterIndex(juce::StringRef name) {
	auto it = std::find_if(ALL_PARAMETERS.begin(), ALL_PARAMETERS.end(),
	[name](ParameterDef const &pd){
		return pd.displayName.equalsIgnoreCase(name);
	});
	if (it != ALL_PARAMETERS.end()){
		size_t index = std::distance(ALL_PARAMETERS.begin(), it);
		return index;
	}
	jassertfalse;
	return 0; // just to avoid warning about not returning for all control paths
}
inline juce::StringArray getAllParameterNames() {
	juce::StringArray a;
	for (auto const &pd : ALL_PARAMETERS){
		a.add(pd.displayName);
	}
	return a;
}

}	// namespace nvs::param
