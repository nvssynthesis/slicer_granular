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

enum class params_e {
	position,
	speed,
	duration,
	skew,
	pan,
	// these should appear as little dials below the main controls
	pos_randomness,
	speed_randomness,
	dur_randomness,
	skew_randomness,
	pan_randomness,
	
	count
};

enum class param_category_e {
	main,
	not_ready,
	random,
	count
};

static const inline std::map<params_e, param_category_e>
paramCategoryMap {
	{params_e::position, param_category_e::main},
	{params_e::speed, param_category_e::main},
	{params_e::duration, param_category_e::main},
	{params_e::skew, param_category_e::main},
	{params_e::pan, param_category_e::main},
	{params_e::pos_randomness, param_category_e::random},
	{params_e::speed_randomness, param_category_e::random},
	{params_e::dur_randomness, param_category_e::random},
	{params_e::skew_randomness, param_category_e::random},
	{params_e::pan_randomness, param_category_e::random}
};

inline bool isMainParam(params_e p){
	return paramCategoryMap.at(p) == param_category_e::main;
}
inline bool isParamOfCategory(params_e p, param_category_e c){
	return paramCategoryMap.at(p) == c;
}

enum class param_elem_e {
	min = 0,
	max,
	interval,
	skewFactor,
	symmetrical,
	defaultVal,
	name,
	count
};

// 				   min,   max,  spacing, skew, symmetrical, default, name
typedef std::tuple<float, float, float,  float,    bool,    float,   std::string> paramPropsTuple;

static constexpr float randSkew {0.33f};

static const inline  std::map<params_e, paramPropsTuple> paramMap {
	{params_e::position,{0.f, 1.f, 0.f, 1.f, false, 0.f, "Position"}},
	{params_e::speed, 	{0.1f, 1000.f, 0.f, 0.3f, false, 10.f, "Speed"}},
	{params_e::duration, {0.1f, 10000.f, 0.f, 0.42f, false, 100.f, "Duration"}},
	{params_e::skew, {0.001f, 0.999f, 0.f, 1.f, false, 0.5f, "Skew"}},
	{params_e::pan, {0.f, 1.f, 0.f, 1.f, false, 0.5f, "Pan Width"}},
	
	{params_e::pos_randomness,	{-1.f, 1.f, 0.f, randSkew, true, 0.f, "Position Randomness"}},
	{params_e::speed_randomness,{-1.f, 1.f, 0.f, randSkew, true, 0.f, "Speed Randomness"}},
	{params_e::dur_randomness, 	{-1.f, 1.f, 0.f, randSkew, true, 0.f, "Duration Randomness"}},
	{params_e::skew_randomness, {-1.f, 1.f, 0.f, randSkew, true, 0.f, "Skew Randomness"}},
	{params_e::pan_randomness, 	{0.f, 1.f, 0.f, randSkew, false, 0.f, "Pan Randomness"}}
};



[[maybe_unused]]
static auto getParamName(params_e p){
	paramPropsTuple tup = paramMap.at(p);
	return std::get<static_cast<size_t>(param_elem_e::name)>(tup);
}

[[maybe_unused]]
static auto getParamDefault(params_e p){
	paramPropsTuple tup = paramMap.at(p);
	return std::get<static_cast<size_t>(param_elem_e::defaultVal)>(tup);
}

template <params_e p, param_elem_e e>
static auto getParamElement(){
	paramPropsTuple tup = paramMap.at(p);
	return std::get<static_cast<size_t>(e)>(tup);
}

// why do it this way? to get either norm range for double or float, since it can't convert...
// this version cannot be placed within a lambda until c++20
template<params_e p, typename float_t>
juce::NormalisableRange<float_t> getNormalizableRange(){
	paramPropsTuple const tup = paramMap.at(p);
	
	using std::get;
	
	juce::NormalisableRange<float_t> range(static_cast<float_t>(getParamElement<p, param_elem_e::min>()),	// min
	   static_cast<float_t>(getParamElement<p, param_elem_e::max>()),	// max
	   static_cast<float_t>(getParamElement<p, param_elem_e::interval>()),	// interval
	   static_cast<float_t>(getParamElement<p, param_elem_e::skewFactor>()),	// skewFactor
	   static_cast<float_t>(getParamElement<p, param_elem_e::symmetrical>())	// symmetricSkew
	);
	
	return range;
}

template<typename float_t>
juce::NormalisableRange<float_t> getNormalizableRange(params_e p){
	paramPropsTuple const tup = paramMap.at(p);
	
	using std::get;
	
	juce::NormalisableRange<float_t> range(static_cast<float_t>(get<static_cast<size_t>(param_elem_e::min)>(tup)),	// min
	   static_cast<float_t>(get<static_cast<size_t>(param_elem_e::max)>(tup)),	// max
	   static_cast<float_t>(get<static_cast<size_t>(param_elem_e::interval)>(tup)),	// interval
	   static_cast<float_t>(get<static_cast<size_t>(param_elem_e::skewFactor)>(tup)),	// skewFactor
	   static_cast<float_t>(get<static_cast<size_t>(param_elem_e::symmetrical)>(tup))	// symmetricSkew
	);
	
	return range;
}
