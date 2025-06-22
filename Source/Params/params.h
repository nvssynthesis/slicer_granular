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

enum class ParameterType {
	Float,
	Choice
};

struct ParameterDef {
	juce::String ID;
	juce::String displayName;  // for UI display (can differ from internal name)
	juce::String groupName;
	juce::String unitSuffix = "";  // e.g., "dB", "Hz", "%"
	juce::String subGroupName = "";
	
	struct FloatParamElements {
		// core range values
		float min, max, defaultVal;
		float interval = 0.0f;  // step size (0 = continuous)
		float skew = 1.0f;
		bool symmetrical = false;
		
		// optional range functions
		std::function<float(float,float,float)> convertFrom0To1 = nullptr;
		std::function<float(float,float,float)> convertTo0To1 = nullptr;
		std::function<float(float,float,float)> snapToLegalValue = nullptr;
		
		std::function<juce::String (float, int)> stringFromValue = nullptr;
		std::function<float (const juce::String&)> valueFromString = nullptr;
		
		int numDecimalPlaces = 2;
	};
	
	struct ChoiceParamElements {
		juce::StringArray choices;
		int defaultChoiceIndex = 0;
	};
	
	std::variant<FloatParamElements, ChoiceParamElements> elementsVar;
	
	ParameterType getParameterType() const {
		if (std::holds_alternative<FloatParamElements>(elementsVar)){
			return ParameterType::Float;
		}
		else if (std::holds_alternative<ChoiceParamElements>(elementsVar)){
			return ParameterType::Choice;
		}
		jassertfalse;
		return ParameterType::Float;
	}
	
	bool hasSubGroup() const { return !subGroupName.isEmpty(); }
	
	// convenience constructors for common parameter types
	static ParameterDef linear(juce::StringRef name, juce::StringRef displayName,
							   juce::StringRef groupName,
							   float min=0.f, float max=1.f, float defaultVal=0.f,
							   juce::StringRef unitSuffix = "", float interval = 0.0f,
							   juce::StringRef subGroupName = "");
	
	static ParameterDef percent(juce::StringRef ID, juce::StringRef displayName,
											juce::StringRef groupName,
								float min=0.f, float max=1.f, float defaultVal=0.f,
								juce::StringRef subGroupName = "",
								float skew=1.f, bool useSymmetricSkew=false);
	
	static ParameterDef skewed(juce::StringRef name, juce::StringRef displayName,
									juce::StringRef groupName,
									float min=0.f, float max=1.f, float defaultVal=0.f,
									juce::StringRef unitSuffix = "",
									float skew=0.3f, bool useSymmetricSkew=false,
								   juce::StringRef subGroupName = "");
	
	static ParameterDef decibel(juce::StringRef name, juce::StringRef displayName,
								juce::StringRef groupName,
								float minDB, float maxDB, float defaultDB=0.f,
								juce::StringRef subGroupName = "");
	
	template<typename T>
	juce::NormalisableRange<T> createNormalisableRange() const {
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
					  "T must be float or double");
		auto const fpe = std::get<FloatParamElements>(elementsVar);
		
		if (fpe.convertFrom0To1 && fpe.convertTo0To1) {
			auto convertFrom0To1Copy = fpe.convertFrom0To1;
			auto convertTo0To1Copy = fpe.convertTo0To1;
			auto stringFromValueCopy = fpe.stringFromValue;
			auto valueFromStringCopy = fpe.valueFromString;
			
			auto from0To1 = [convertFrom0To1Copy](T start, T end, T val) -> T {
				return static_cast<T>(convertFrom0To1Copy(float(start), float(end), float(val)));
			};
			auto to0To1 = [convertTo0To1Copy](T start, T end, T val) -> T {
				return static_cast<T>(convertTo0To1Copy(float(start), float(end), float(val)));
			};
			auto stringFromValue = [stringFromValueCopy](T val, int numDecimalPlaces) -> juce::String {
				return stringFromValueCopy(val, numDecimalPlaces);
			};
			auto valueFromString = [valueFromStringCopy](juce::String &s) -> T {
				return valueFromStringCopy(s);
			};
			
			std::function<T(T,T,T)> snapFunc = nullptr;
			if (fpe.snapToLegalValue) {
				auto snapToLegalValueCopy = fpe.snapToLegalValue;
				snapFunc = [snapToLegalValueCopy](T start, T end, T val) -> T {
					return static_cast<T>(snapToLegalValueCopy(float(start), float(end), float(val)));
				};
			}
			
			return juce::NormalisableRange<T>(static_cast<T>(fpe.min), static_cast<T>(fpe.max),
											  from0To1, to0To1, snapFunc);
		} else {
			// Standard constructor
			return juce::NormalisableRange<T>(static_cast<T>(fpe.min), static_cast<T>(fpe.max),
											  static_cast<T>(fpe.interval), static_cast<T>(fpe.skew),
											  fpe.symmetrical);
		}
	}
	
	// convenience methods for JUCE components
	juce::NormalisableRange<float> getFloatRange() const { return createNormalisableRange<float>(); }
	juce::NormalisableRange<double> getDoubleRange() const { return createNormalisableRange<double>(); }
};

inline ParameterDef ParameterDef::linear(juce::StringRef ID, juce::StringRef displayName,
										juce::StringRef groupName,
										float min, float max, float defaultVal,
										juce::StringRef unitSuffix, float interval,
										juce::StringRef subGroupName) {
	ParameterDef param;
	param.ID			= ID;
	param.displayName	= displayName;
	param.groupName		= groupName;
	param.unitSuffix	= unitSuffix;
	param.subGroupName	= subGroupName;
	param.elementsVar = FloatParamElements
	{
		.min 			= min,
		.max			= max,
		.defaultVal 	= defaultVal,
		.interval		= interval
	};
	
	return param;
}

inline ParameterDef ParameterDef::skewed(juce::StringRef ID, juce::StringRef displayName,
											juce::StringRef groupName,
											float min, float max, float defaultVal,
											juce::StringRef unitSuffix,
											float skew, bool symmetricSkew,
											juce::StringRef subGroupName) {
	ParameterDef param;
	param.ID			= ID;
	param.displayName	= displayName;
	param.groupName		= groupName;
	param.unitSuffix	= unitSuffix;
	param.subGroupName	= subGroupName;
	param.elementsVar = FloatParamElements
	{
		.min 			= min,
		.max			= max,
		.defaultVal 	= defaultVal,
		.skew			= skew,
		.symmetrical	= symmetricSkew,
		.interval		= 0.0f
	};
	
	return param;
}

inline ParameterDef ParameterDef::percent(juce::StringRef ID, juce::StringRef displayName,
										juce::StringRef groupName,
										float min, float max, float defaultVal,
										juce::StringRef subGroupName,
										float skew, bool symmetricSkew) {
	ParameterDef param;
	param.ID			= ID;
	param.displayName	= displayName;
	param.groupName		= groupName;
	param.unitSuffix	= "%";
	param.subGroupName	= subGroupName;
	int numDecimals = 1;
	
	auto const check = [](float x){
		jassert ((0.0 <= x) and (x <= 1.0));	// internally the param is 0-1, not 0-100
	};
	check(min); check(max); check(defaultVal);
	
	param.elementsVar = FloatParamElements
	{
		.min 			= min,
		.max			= max,
		.defaultVal 	= defaultVal,
		.skew			= skew,
		.symmetrical	= symmetricSkew,
		.interval		= 0.0f,
		.numDecimalPlaces = numDecimals,
		
		.stringFromValue = [suff = param.unitSuffix, numDecimals](float val, int) -> juce::String
		{
			float percentageVal = val * 100.0f;
			return juce::String(percentageVal, numDecimals) + suff;
		},
		.valueFromString = [](juce::String const &text) -> float
		{
			return text.getFloatValue() * 0.01f;
		}
	};
	
	return param;
}

inline ParameterDef ParameterDef::decibel (juce::StringRef ID,
										juce::StringRef displayName,
										juce::StringRef groupName,
										float minDB, float maxDB, float defaultDB,
										juce::StringRef subGroupName)
{
	ParameterDef param;
	param.ID 				= ID;
	param.displayName 		= displayName;
	param.groupName 		= groupName;
	param.unitSuffix 		= " dB";
	param.subGroupName 		= subGroupName;
	
	const float minGain = juce::Decibels::decibelsToGain(minDB, minDB);
	const float maxGain = juce::Decibels::decibelsToGain(maxDB, minDB);
	const float defaultGain = juce::Decibels::decibelsToGain(defaultDB, minDB);
	const float minusInfinityDB = -100.0f;
	
	param.elementsVar = FloatParamElements
	{
		.min = minGain,
		.max = maxGain,
		.defaultVal = defaultGain,
		.numDecimalPlaces = 1,
		
		// Convert normalized [0,1] to dB, then to gain for storage
		.convertFrom0To1 = [minDB, maxDB, minusInfinityDB](float, float, float normVal) -> float
		{
			float dbVal = minDB + normVal * (maxDB - minDB);
			return juce::Decibels::decibelsToGain(dbVal, minusInfinityDB);
		},
		.valueFromString = [minusInfinityDB](juce::String const& text) -> float
		{
			float dbVal = text.getFloatValue();
			return juce::Decibels::decibelsToGain(dbVal, minusInfinityDB);
		},
		
		// Convert gain to dB, then to normalized [0,1]
		.convertTo0To1 = [minDB, maxDB, minusInfinityDB](float, float, float gainVal) -> float
		{
			float dbVal = juce::Decibels::gainToDecibels(gainVal, minusInfinityDB);
			float normalized = (dbVal - minDB) / (maxDB - minDB);
			// Clamp to handle floating point precision issues
			return juce::jlimit(0.0f, 1.0f, normalized);
		},
		.stringFromValue = [minusInfinityDB](float gainVal, int numDecimalPlaces) -> juce::String
		{
			float dbVal = juce::Decibels::gainToDecibels(gainVal, minusInfinityDB);
			return juce::String(dbVal, numDecimalPlaces) + " dB";
		}
	};
	return param;
}

static constexpr float envTimingMin {0.01f};
static constexpr float envTimingMax {8.f};
static constexpr float envTimingSkew {0.5f};

static constexpr float skeps = 5e-3f;	// epsilon for skew

inline const std::vector<ParameterDef> ALL_PARAMETERS = {
	ParameterDef::linear("transpose", 	"Transpose", 	"Main", -60.f,	    60.f,		0.f, 	" semi"),
	ParameterDef::percent("position", "Position", 		"Main",   0.f,	     1.f,		0.f),
	ParameterDef::skewed("speed", "Speed", 			 	"Main",  0.1f, 	  1000.f, 		50.f,	"hz"),
	ParameterDef::percent("duration", "Duration", 	 	"Main", 1e-6f, 	  	 1.f, 		0.1f,	"", 0.3, false),	// percent with skew
	ParameterDef::percent("skew", 	"Skew", 			"Main",	skeps, 	1.f-skeps, 		0.5f),						// percent with clipped range
	ParameterDef::skewed("plateau", "Plateau", 		 	"Main",	0.01f, 		10.f, 		1.f, 	""),
	ParameterDef::percent("pan", 	"Pan", 			 	"Main", 0.f,		 1.f,		0.5f),

	ParameterDef::skewed("transpose_rand", "Transpose Randomness", 		"MainRandom"),
	ParameterDef::skewed("position_rand", "Position Randomness", 		"MainRandom"),
	ParameterDef::skewed("speed_rand", "Speed Randomness", 				"MainRandom"),
	ParameterDef::skewed("duration_rand", "Duration Randomness", 		"MainRandom"),
	ParameterDef::skewed("skew_rand", "Skew Randomness",	 			"MainRandom"),
	ParameterDef::skewed("plateau_rand", "Plateau Randomness", 			"MainRandom"),
	ParameterDef::skewed("pan_rand", "Pan Randomness",		 			"MainRandom", 0.0f, 1.0f, 0.5f),

	ParameterDef::skewed("amp_env_attack", 	"Attack", 	"Amplitude Envelope", envTimingMin, 	envTimingMax, 	0.05f, 	" Seconds"),
	ParameterDef::skewed("amp_env_decay", 	"Decay", 	"Amplitude Envelope", envTimingMin, 	envTimingMax, 	1.0f, 	" Seconds"),
	ParameterDef::percent("amp_env_sustain", "Sustain", "Amplitude Envelope", 	0.f, 				1.f, 		0.85f),
	ParameterDef::skewed("amp_env_release", "Release", 	"Amplitude Envelope", envTimingMin, 	envTimingMax, 	1.0f,	" Seconds"),

	ParameterDef::skewed("scanner_rate",	"Rate",		"Scanner", 				-20.f,				20.f,		0.f,	"Hz", 0.3f, true),
	ParameterDef::percent("scanner_amount",	"Amount",	"Scanner"),
	
#ifdef TSN
	// create TSN navigation and other params here
	ParameterDef::linear("nav_tendency_x", 			"Navigator Tendency X", "Navigator", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_y", 			"Navigator Tendency Y", "Navigator", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_z", 			"Navigator Tendency Z", "Navigator", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_u", 			"Navigator Tendency U", "Navigator", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_v", 			"Navigator Tendency V", "Navigator", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_w", 			"Navigator Tendency W", "Navigator", -1.f, 1.f, 0.f, "", 0.f, "tendency"),

	ParameterDef::linear("nav_selection_sharpness", "Selection Sharpness", "Navigator", -200.f, 2200.f, 10.f, "", 0.f, "selection"),
	
	ParameterDef::percent("nav_lfo_amount", "Amount", 	"Navigator", 0.f, 1.f, 0.f,	"nav_lfo"),
	ParameterDef::percent("nav_lfo_shape", "Shape", 	"Navigator", 0.f, 1.f, 0.f,	"nav_lfo"),
	ParameterDef::skewed("nav_lfo_rate", "Rate", 		"Navigator", 0.1f, 10.f, 0.3f, "Hz", 0.3f, false, "nav_lfo"),
	ParameterDef::skewed("nav_lfo_response", "Response","Navigator", 0.01f, 4.f, 1.f, "", 0.5f, false, "nav_lfo"),
	ParameterDef::skewed("nav_lfo_overshoot", "Overshoot", "Navigator", 0.55f, 24.f, 0.f, "", 0.3f, false, "nav_lfo"),

	ParameterDef::skewed("nav_rwalk_step_size", "Nav Random Walk Step Size", "Navigator", 0.f, 0.1f, 0.f, "", 0.01f, false, "nav_rwalk"),
#endif
	
	ParameterDef::decibel("fx_grain_drive", "Grain Drive", "Fx", -10.f, 40.f, 0.f),
	ParameterDef::decibel("fx_makeup_gain", "Makeup Gain", "Fx", -40.f, 20.f, 0.f)
};


struct ParameterRegistry {
public:
	static std::vector<ParameterDef> getParametersForGroup(juce::StringRef groupName);
	static std::vector<ParameterDef> getParametersForSubGroup(juce::StringRef groupName);
	static const ParameterDef& getParameterByName(juce::StringRef name);
	static const ParameterDef& getParameterByID(juce::StringRef id);
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
inline std::vector<ParameterDef> ParameterRegistry::getParametersForSubGroup(juce::StringRef subGroupName){
	std::vector<ParameterDef> params;
	for (auto const &pd : ALL_PARAMETERS){
		if (pd.subGroupName.equalsIgnoreCase(subGroupName)){
			params.push_back(pd);
		}
	}
	return params;
}
inline const ParameterDef& ParameterRegistry::getParameterByID(juce::StringRef id) {
	auto it = std::find_if(ALL_PARAMETERS.begin(), ALL_PARAMETERS.end(),
	[id](ParameterDef const &pd){
		return pd.ID.equalsIgnoreCase(id);
	});
	if (it != ALL_PARAMETERS.end()){
		return *it;
	}
	jassertfalse;
	return ALL_PARAMETERS[0];	// just to avoid warning about not returning for all control paths
}
inline size_t ParameterRegistry::getParameterIndex(juce::StringRef name) {
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
inline juce::StringArray ParameterRegistry::getAllParameterNames() {
	juce::StringArray a;
	for (auto const &pd : ALL_PARAMETERS){
		a.add(pd.displayName);
	}
	return a;
}

}	// namespace nvs::param
