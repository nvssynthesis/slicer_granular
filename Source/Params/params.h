/*
  ==============================================================================

    params.h
    Created: 16 Jun 2023 5:37:13pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include <tuple>
#include <JuceHeader.h>

#include "StringAxiom.h"

#ifdef TSN
#include "../tsn-analyzer/Source/lib/StringAxiom.h"
#include "Navigation/Navigator.h"
#include "FeatureOperations.h"
#include "StatisticsOperations.h"
#endif

/*** TODO:
 -octave
 -fine tuning
 -master volume
 */
namespace nvs::param {

enum class ParameterType {
	Float,
	Choice
};

struct ParameterDef {
	String ID;
	String displayName;  // for UI display (can differ from internal name)
	String groupName;
	String unitSuffix = "";  // e.g., "dB", "Hz", "%"
	String subGroupName = "";
    String deprecatedID = "";
	
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
		
		std::function<String (float, int)> stringFromValue = nullptr;
		std::function<float (const String&)> valueFromString = nullptr;
		
		int numDecimalPlaces = 2;
	};
	
	struct ChoiceParamElements {
		StringArray choices;
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
	static ParameterDef linear(StringRef ID, StringRef displayName,
							   StringRef groupName,
							   float min=0.f, float max=1.f, float defaultVal=0.f,
							   StringRef unitSuffix = "", float interval = 0.0f,
							   StringRef subGroupName = "");
	
	static ParameterDef percent(StringRef ID, StringRef displayName,
											StringRef groupName,
								float min=0.f, float max=1.f, float defaultVal=0.f,
								StringRef subGroupName = "",
								float skew=1.f, bool useSymmetricSkew=false, StringRef deprecatedID = "");
	
	static ParameterDef skewed(StringRef ID, StringRef displayName,
									StringRef groupName,
									float min=0.f, float max=1.f, float defaultVal=0.f,
									StringRef unitSuffix = "",
									float skew=0.3f, bool useSymmetricSkew=false,
								   StringRef subGroupName = "", StringRef deprecatedID = "");
	
	static ParameterDef decibel(StringRef ID, StringRef displayName,
								StringRef groupName,
								float minDB, float maxDB, float defaultDB=0.f,
								StringRef subGroupName = "");

    static ParameterDef choice(StringRef ID, StringRef displayName,
                            StringRef groupName,
                            const StringArray &elements,
                            int defaultChoiceIdx=0,
                            StringRef subGroupName = "");

	template<typename T>
	NormalisableRange<T> createNormalisableRange() const {
		static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>,
					  "T must be float or double");
		auto const fpe = std::get<FloatParamElements>(elementsVar);
		
		if (fpe.convertFrom0To1 && fpe.convertTo0To1) {
			auto convertFrom0To1Copy = fpe.convertFrom0To1;
			auto convertTo0To1Copy = fpe.convertTo0To1;
			auto stringFromValueCopy = fpe.stringFromValue;
			auto valueFromStringCopy = fpe.valueFromString;
			
			auto from0To1 = [convertFrom0To1Copy](T start, T end, T val) -> T {
				return static_cast<T>(convertFrom0To1Copy(static_cast<float>(start), static_cast<float>(end), static_cast<float>(val)));
			};
			auto to0To1 = [convertTo0To1Copy](T start, T end, T val) -> T {
				return static_cast<T>(convertTo0To1Copy(static_cast<float>(start), static_cast<float>(end), static_cast<float>(val)));
			};
			auto stringFromValue = [stringFromValueCopy](T val, const int numDecimalPlaces) -> String {
				return stringFromValueCopy(static_cast<float>(val), numDecimalPlaces);
			};
			auto valueFromString = [valueFromStringCopy](const String &s) -> T {
				return valueFromStringCopy(s);
			};
			
			std::function<T(T,T,T)> snapFunc = nullptr;
			if (fpe.snapToLegalValue) {
				auto snapToLegalValueCopy = fpe.snapToLegalValue;
				snapFunc = [snapToLegalValueCopy](T start, T end, T val) -> T {
					return static_cast<T>(snapToLegalValueCopy(static_cast<float>(start), static_cast<float>(end), static_cast<float>(val)));
				};
			}
			
			return NormalisableRange<T>(static_cast<T>(fpe.min), static_cast<T>(fpe.max),
											  from0To1, to0To1, snapFunc);
		} else {
			// Standard constructor
			return NormalisableRange<T>(static_cast<T>(fpe.min), static_cast<T>(fpe.max),
											  static_cast<T>(fpe.interval), static_cast<T>(fpe.skew),
											  fpe.symmetrical);
		}
	}
	
	// convenience methods for JUCE components
	NormalisableRange<float> getFloatRange() const { return createNormalisableRange<float>(); }
	NormalisableRange<double> getDoubleRange() const { return createNormalisableRange<double>(); }
};

inline ParameterDef ParameterDef::linear(const StringRef ID, StringRef displayName,
										const StringRef groupName,
										const float min, const float max, const float defaultVal,
										const StringRef unitSuffix, const float interval,
										const StringRef subGroupName) {
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

inline ParameterDef ParameterDef::skewed(const StringRef ID, const StringRef displayName,
											const StringRef groupName,
											const float min, const float max, const float defaultVal,
											const StringRef unitSuffix,
											const float skew, const bool useSymmetricSkew,
											const StringRef subGroupName,
											const StringRef deprecatedID) {
	ParameterDef param;
	param.ID			= ID;
	param.displayName	= displayName;
	param.groupName		= groupName;
	param.unitSuffix	= unitSuffix;
	param.subGroupName	= subGroupName;
    param.deprecatedID	= deprecatedID;
	param.elementsVar = FloatParamElements
	{
		.min 			= min,
		.max			= max,
		.defaultVal 	= defaultVal,
		.skew			= skew,
		.symmetrical	= useSymmetricSkew,
		.interval		= 0.0f
	};
	
	return param;
}

inline ParameterDef ParameterDef::percent(const StringRef ID, const StringRef displayName,
										const StringRef groupName,
										const float min, const float max, const float defaultVal,
										const StringRef subGroupName,
										const float skew, const bool useSymmetricSkew,
										const StringRef deprecatedID) {
	ParameterDef param;
	param.ID			= ID;
	param.displayName	= displayName;
	param.groupName		= groupName;
	param.unitSuffix	= "%";
	param.subGroupName	= subGroupName;
    param.deprecatedID	= deprecatedID;
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
		.symmetrical	= useSymmetricSkew,
		.interval		= 0.0f,
		.numDecimalPlaces = numDecimals,
		
		.stringFromValue = [suff = param.unitSuffix, numDecimals](const float val, int) -> String
		{
			float percentageVal = val * 100.0f;
			return String(percentageVal, numDecimals) + suff;
		},
		.valueFromString = [](String const &text) -> float
		{
			return text.getFloatValue() * 0.01f;
		}
	};
	
	return param;
}

inline ParameterDef ParameterDef::decibel (const StringRef ID,
										const StringRef displayName,
										const StringRef groupName,
										const float minDB, const float maxDB, const float defaultDB,
										const StringRef subGroupName)
{
	ParameterDef param;
	param.ID 				= ID;
	param.displayName 		= displayName;
	param.groupName 		= groupName;
	param.unitSuffix 		= " dB";
	param.subGroupName 		= subGroupName;
	
	const float minGain = Decibels::decibelsToGain(minDB, minDB);
	const float maxGain = Decibels::decibelsToGain(maxDB, minDB);
	const float defaultGain = Decibels::decibelsToGain(defaultDB, minDB);
    constexpr float minusInfinityDB = -100.0f;
	
	param.elementsVar = FloatParamElements
	{
		.min = minGain,
		.max = maxGain,
		.defaultVal = defaultGain,
		.numDecimalPlaces = 1,
		
		// Convert normalized [0,1] to dB, then to gain for storage
		.convertFrom0To1 = [minDB, maxDB](float, float, const float normVal) -> float
		{
			const float dbVal = minDB + normVal * (maxDB - minDB);
			return Decibels::decibelsToGain(dbVal, minusInfinityDB);
		},
		.valueFromString = [](String const& text) -> float
		{
			const float dbVal = text.getFloatValue();
			return Decibels::decibelsToGain(dbVal, minusInfinityDB);
		},
		
		// Convert gain to dB, then to normalized [0,1]
		.convertTo0To1 = [minDB, maxDB](float, float, const float gainVal) -> float
		{
			const float dbVal = Decibels::gainToDecibels(gainVal, minusInfinityDB);
			const float normalized = (dbVal - minDB) / (maxDB - minDB);
			// Clamp to handle floating point precision issues
			return jlimit(0.0f, 1.0f, normalized);
		},
		.stringFromValue = [](const float gainVal, const int numDecimalPlaces) -> String
		{
			const float dbVal = Decibels::gainToDecibels(gainVal, minusInfinityDB);
			return String(dbVal, numDecimalPlaces) + " dB";
		}
	};
	return param;
}
inline ParameterDef ParameterDef::choice(const StringRef ID,
                            const StringRef displayName,
                            const StringRef groupName,
                            const StringArray &elements,
                            const int defaultChoiceIdx,
                            const StringRef subGroupName) {
    ParameterDef param;
    param.ID			= ID;
    param.displayName	= displayName;
    param.groupName		= groupName;
    param.subGroupName	= subGroupName;
    param.elementsVar = ChoiceParamElements
    {
        .choices 		= elements,
        .defaultChoiceIndex = defaultChoiceIdx
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
	ParameterDef::skewed("speed", "Speed", 			 	"Main",  0.1f, 	 10000.f, 		50.f,	"hz"),
	ParameterDef::percent("density", "Density", 	 	"Main", 1e-4f, 	  	 1.f, 		0.1f,	"", 1.0f, false, "duration"),	// percent with skew
	ParameterDef::linear("skew", 	"Skew", 			"Main",	-10.f, 	    10.f, 		0.f),						// percent with clipped range
	ParameterDef::linear("plateau", "Plateau", 		 	"Main",	-10.f, 		10.f, 		0.f),
	ParameterDef::percent("pan", 	"Pan", 			 	"Main",   0.f,		 1.f,		0.5f),

    ParameterDef::choice(nvs::axiom::frequency_randomization_mode, "Frequency Randomization Mode", "MainHidden", {nvs::axiom::Continuous, nvs::axiom::Octaves}, 0, ""),

	ParameterDef::skewed("transpose_rand", "Transpose Randomness", 		"MainRandom"),
	ParameterDef::skewed("position_rand", "Position Randomness", 		"MainRandom"),
	ParameterDef::skewed("speed_rand", "Speed Randomness", 				"MainRandom"),
	ParameterDef::skewed("density_rand", "Density Randomness", 		    "MainRandom", 0, 1, 0, "", 0.3, false, "", "duration_rand"),
	ParameterDef::skewed("skew_rand", "Skew Randomness",	 			"MainRandom"),
	ParameterDef::skewed("plateau_rand", "Plateau Randomness", 			"MainRandom"),
	ParameterDef::skewed("pan_rand", "Pan Randomness",		 			"MainRandom", 0.0f, 1.0f, 0.5f),

	ParameterDef::skewed("amp_env_attack", 	"Attack", 	"Amplitude Envelope", envTimingMin, 	envTimingMax, 	0.05f, 	" Seconds"),
	ParameterDef::skewed("amp_env_decay", 	"Decay", 	"Amplitude Envelope", envTimingMin, 	envTimingMax, 	1.0f, 	" Seconds"),
	ParameterDef::percent("amp_env_sustain", "Sustain", "Amplitude Envelope", 	0.f, 				1.f, 		0.85f),
	ParameterDef::skewed("amp_env_release", "Release", 	"Amplitude Envelope", envTimingMin, 	envTimingMax, 	1.0f,	" Seconds"),

    ParameterDef::linear("scanner_shape", "Shape",      "Scanner",              0.0,                   1.0,    0.25 /* saw is 1/4*/),
	ParameterDef::skewed("scanner_rate",	"Rate",		"Scanner", 				-20.f,				20.f,		0.f,	"Hz", 0.3f, true),
	ParameterDef::percent("scanner_amount",	"Amount",	"Scanner"),
	
#ifdef TSN
	// create TSN navigation and other params here
    ParameterDef::choice("navigator_type", "Navigator Type", "TSN", timbrespace::getNavigatorTypeArray()),

    // add: higher3Dweight (float), pointSelectionMethod (triangulation vs distance)

	ParameterDef::linear("nav_tendency_x", 			"Navigator Tendency X", "TSN", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_y", 			"Navigator Tendency Y", "TSN", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_z", 			"Navigator Tendency Z", "TSN", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_u", 			"Navigator Tendency U", "TSN", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_v", 			"Navigator Tendency V", "TSN", -1.f, 1.f, 0.f, "", 0.f, "tendency"),
	ParameterDef::linear("nav_tendency_w", 			"Navigator Tendency W", "TSN", -1.f, 1.f, 0.f, "", 0.f, "tendency"),

    ParameterDef::choice(nvs::axiom::tsn::pitchify, "Pitchify", "MainHidden", {"On", "Off"}, 0, ""),

    ParameterDef::choice(nvs::axiom::tsn::DRMode, "DR Mode", "TSN", {"None", "PaCMAP"}, 0, "timbre_space"),
	ParameterDef::linear(nvs::axiom::tsn::histogram_equalization, "Histogram Equalization", "TSN", 0.f, 1.f, 0.f, "", 0.f, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::x_axis, "X Axis", "TSN", analysis::getFeaturesStringArray(), 0, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::y_axis, "Y Axis", "TSN", analysis::getFeaturesStringArray(), 1, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::z_axis, "Z Axis", "TSN", analysis::getFeaturesStringArray(), 2, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::u_axis, "U Axis", "TSN", analysis::getFeaturesStringArray(), 3, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::v_axis, "V Axis", "TSN", analysis::getFeaturesStringArray(), 4, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::statistic, "statistic", "TSN", analysis::getStatisticsStringArray(), 0, "timbre_space"),
    ParameterDef::choice(nvs::axiom::tsn::decorrelateFromPitchAndLoudness, "Decorrelate", "TSN", {"Not Decorrelated", "Decorrelated"}, 1, "timbre_space"),

    ParameterDef::choice(nvs::axiom::tsn::filtered_feature, "Filtered Feature", "TSN", analysis::getFeaturesStringArray(), 16 /*spectral flatness for now, spectral entropy when available */, "timbre_space_cull"),
    ParameterDef::linear(nvs::axiom::tsn::filtered_feature_min, "Filtered Feature Minimum", "TSN", 0.f, 1.f, 0.f, "", 0.f, "timbre_space_cull"),
    ParameterDef::linear(nvs::axiom::tsn::filtered_feature_max, "Filtered Feature Maximum", "TSN", 0.f, 1.f, 1.f, "", 0.f, "timbre_space_cull"),

    ParameterDef::skewed("nav_manual_response", "Response","TSN", 0.01f, 4.f, 1.f, "", 0.5f, false, "nav_manual"),
    ParameterDef::skewed("nav_manual_overshoot", "Overshoot", "TSN", 0.55f, 24.f, 0.f, "", 0.3f, false, "nav_manual"),

	ParameterDef::percent("nav_lfo_shape", "Shape", 	"TSN", 0.f, 1.f, 0.f,	"nav_lfo"),
	ParameterDef::skewed("nav_lfo_rate", "Rate", 		"TSN", 0.01f, 10.f, 0.3f, "Hz", 0.3f, false, "nav_lfo"),

	ParameterDef::skewed("nav_rwalk_step_size", "Nav Random Walk Step Size", "TSN", 0.f, 0.2f, 0.1f, "", 0.5f, false, "nav_rwalk"),

    ParameterDef::linear("nav_lorenz_a",  "a", "TSN", 8.f, 12.f, 10.f, "", 0.f, "nav_lorenz"),
    ParameterDef::linear("nav_lorenz_b",  "b", "TSN", 1.0f, 350.f, 28.f, "", 0.f, "nav_lorenz"),
    ParameterDef::linear("nav_lorenz_c",  "c", "TSN", 1.5f, 4.f, 2.67f, "", 0.f, "nav_lorenz"),
    ParameterDef::skewed("nav_lorenz_d_t",  "d_t", "TSN", 0.f, 0.01f, 0.005f, "", 0.33f, false, "nav_lorenz"),

    ParameterDef::skewed("nav_hyperchaos_a", "a", "TSN", 10e-5f, 10e-1f, 10e-4f, "", 0.2f, false, "nav_hyperchaos"),
    ParameterDef::skewed("nav_hyperchaos_b", "b", "TSN", 10e-5f, 10e-1f, 10e-4f, "", 0.2f, false, "nav_hyperchaos"),
    ParameterDef::skewed("nav_hyperchaos_d_t", "d_t", "TSN", 0.f, 0.5f, 0.005f, "", 0.33f, false, "nav_hyperchaos"),

    ParameterDef::linear("nav_rotation_x", "Roll", "TSN", 0.f, 1.f, 0.f, "", 0.f, "nav_common"),
    ParameterDef::linear("nav_rotation_y", "Pitch", "TSN", 0.f, 1.f, 0.f, "", 0.f, "nav_common"),
    ParameterDef::linear("nav_rotation_z", "Yaw", "TSN", 0.f, 1.f, 0.f, "", 0.f, "nav_common"),
    ParameterDef::skewed("nav_scaling", "Scaling", "TSN", 0.f, 8.f, 1.f, "", 0.25f, false, "nav_common"),

#endif

    ParameterDef::linear("fx_grain_normalize", "Grain Normalization", "Fx", 0, 1, 0.1f, "", 0, "normalization"),
    // tempting to use ParameterDef::decibel, but we actually want to internally store these params as dB to simply add them and convert the final randomized result to ratio
	ParameterDef::linear("fx_grain_drive", "Grain Drive", "Fx", -10.f, 60.f, 0.f, "dB", 0.1, "drive"),
	ParameterDef::linear("fx_grain_drive_rand", "Grain Drive Randomness", "Fx", 0.f, 30.f, 0.f, "dB", 0.1, "drive"),
	ParameterDef::decibel("fx_makeup_gain", "Makeup Gain", "Fx", -40.f, 20.f, 0.f, "drive")
};


struct ParameterRegistry {
public:
	static std::vector<ParameterDef> getParametersForGroup(StringRef groupName);
	static std::vector<ParameterDef> getParametersForSubGroup(StringRef groupName);
	static const ParameterDef& getParameterByName(StringRef name);
	static const ParameterDef& getParameterByID(StringRef id);
	static StringArray getAllParameterNames();
	static size_t getParameterIndex(StringRef name);
};
inline std::vector<ParameterDef> ParameterRegistry::getParametersForGroup(StringRef groupName){
	std::vector<ParameterDef> params;
	for (auto const &pd : ALL_PARAMETERS){
		if (pd.groupName.equalsIgnoreCase(groupName)){
			params.push_back(pd);
		}
	}
	return params;
}
inline std::vector<ParameterDef> ParameterRegistry::getParametersForSubGroup(StringRef subGroupName){
	std::vector<ParameterDef> params;
	for (auto const &pd : ALL_PARAMETERS){
		if (pd.subGroupName.equalsIgnoreCase(subGroupName)){
			params.push_back(pd);
		}
	}
	return params;
}
inline const ParameterDef& ParameterRegistry::getParameterByID(StringRef id) {
	auto it = std::ranges::find_if(ALL_PARAMETERS,
                                   [id](ParameterDef const &pd){
                                       return pd.ID.equalsIgnoreCase(id);
                                   });
	if (it != ALL_PARAMETERS.end()){
		return *it;
	}
    it = std::ranges::find_if(ALL_PARAMETERS,
                               [id](ParameterDef const &pd){
                                   return pd.deprecatedID.equalsIgnoreCase(id);
                               });
    if (it != ALL_PARAMETERS.end()){
        return *it;
    }
	jassertfalse;
	return ALL_PARAMETERS[0];	// just to avoid warning about not returning for all control paths
}
inline size_t ParameterRegistry::getParameterIndex(StringRef name) {
	const auto it = std::ranges::find_if(ALL_PARAMETERS,
                                   [name](ParameterDef const &pd){
                                       return pd.displayName.equalsIgnoreCase(name);
                                   });
	if (it != ALL_PARAMETERS.end()){
		const auto index = std::distance(ALL_PARAMETERS.begin(), it);
	    jassert(index >= 0);
		return static_cast<size_t>(index);
	}
	jassertfalse;
	return 0; // just to avoid warning about not returning for all control paths
}
inline StringArray ParameterRegistry::getAllParameterNames() {
	StringArray a;
	for (auto const &pd : ALL_PARAMETERS){
		a.add(pd.displayName);
	}
	return a;
}

}	// namespace nvs::param
