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

enum class params_e {
	transpose,
	position,
	speed,
	duration,
	skew,
	plateau,
	pan,
	// these should appear as little dials below the main controls
	transp_randomness,
	pos_randomness,
	speed_randomness,
	dur_randomness,
	skew_randomness,
	plat_randomness,
	pan_randomness,
	
	count_main_granular_params,
	
	amp_attack,
	amp_decay,
	amp_sustain,
	amp_release,
	
	count_envelope_params,

    pos_scan_amount,
    pos_scan_rate,
    count_pos_scan_params
	
#ifdef TSN
,	nav_tendency_x,	// left-right
	nav_tendency_y,	// up-down
	nav_tendency_z,	// distance (shown by size and maybe shading)
	nav_tendency_u,	// color
	nav_tendency_v,	// color
	nav_tendency_w,	// color
	count_nav_tendency_params,

	nav_lfo_2d_amount,
	nav_lfo_2d_rate,
	nav_lfo_2d_response,
	nav_lfo_2d_overshoot,
	count_lfo_2d_params,
	
	nav_random_walk_step_size,
//	nav_random_walk_compensation,
//	nav_random_walk_compensation_curve,
	count_random_walk_params
#endif
};

enum class navigator_category_e {
	lfo_2d,
	random_walk
};

const std::map<navigator_category_e, std::vector<params_e>> categoryToParams
{
	{ navigator_category_e::lfo_2d, {
		params_e::nav_lfo_2d_amount,
		params_e::nav_lfo_2d_rate,
		params_e::nav_lfo_2d_response,
		params_e::nav_lfo_2d_overshoot
	}},
	{ navigator_category_e::random_walk, {
		params_e::nav_random_walk_step_size
//		params_e::nav_random_walk_compensation,
//		params_e::nav_random_walk_compensation_curve
	}}
};

inline params_e mainToRandom(params_e mainParam){
	int mainInt = static_cast<int>(mainParam);
	constexpr int firstRandomInt = static_cast<int>(params_e::transp_randomness);
	assert(mainInt < firstRandomInt);
	constexpr int diff = firstRandomInt - static_cast<int>(params_e::transpose);
	int randomParamIdx = mainInt + diff;
	assert(randomParamIdx < static_cast<int>(params_e::count_main_granular_params));
	return static_cast<params_e>(randomParamIdx);
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

static constexpr float envTimingMin {0.01f};
static constexpr float envTimingMax {8.f};
static constexpr float envTimingSkew {0.5f};
static const inline  std::map<params_e, paramPropsTuple> paramMap {
	// 				   		min,   max,       spacing, skew, symmetrical, default, name
	{params_e::transpose,	{-60.f, 60.f, 		0.f, 	1.f, 	true, 	0.f, 	"Transpose"}},
	{params_e::position,	{0.f, 	1.f, 		0.f, 	1.f, 	false, 	0.f, 	"Position"}},
	{params_e::speed, 		{0.1f, 	1000.f, 	0.f, 	0.3f, 	false, 	50.f, 	"Speed"}},
	{params_e::duration, 	{1e-6f, 1.f, 		0.f, 	0.4f, 	false, 	0.1f, 	"Duration"}},
	{params_e::skew, 		{5e-3f, 1.f-5e-3f,  0.f, 	1.f, 	false, 	0.5f, 	"Skew"}},
	{params_e::plateau, 	{0.01f, 10.f, 		0.f, 	0.5f, 	false, 	1.f, 	"Plateau"}},
	{params_e::pan, 		{0.f, 	1.f, 		0.f, 	1.f, 	false, 	0.5f, 	"Panning"}},
	// 				   				min,  max, spacing, skew, 	sym,  default, name
	{params_e::transp_randomness,	{0.f, 1.f, 0.f, randSkew, 	false, 0.f, "Transpose Randomness"}},
	{params_e::pos_randomness,		{0.f, 1.f, 0.f, randSkew, 	false, 0.f, "Position Randomness"}},
	{params_e::speed_randomness,	{0.f, 1.f, 0.f, randSkew, 	false, 0.1f, "Speed Randomness"}},
	{params_e::dur_randomness, 		{0.f, 1.f, 0.f, randSkew, 	false, 0.1f, "Duration Randomness"}},
	{params_e::skew_randomness, 	{0.f, 1.f, 0.f, randSkew, 	false, 0.f, "Skew Randomness"}},
	{params_e::plat_randomness, 	{0.f, 1.f, 0.f, randSkew, 	false, 0.f, "Plateau Randomness"}},
	{params_e::pan_randomness, 		{0.f, 1.f, 0.f,	 0.75f, 	false, 0.23f, "Pan Randomness"}},
	// 				   		min,   			max,  		spacing,  skew, 	symmetrical, default, name
	{params_e::amp_attack,	{envTimingMin, envTimingMax, 0.f, 	envTimingSkew, 	false, 	0.05f, 	"Amp Env Attack"}},
	{params_e::amp_decay,	{envTimingMin, envTimingMax, 0.f, 	envTimingSkew, 	false, 	0.5f, 	"Amp Env Decay"}},
	{params_e::amp_sustain,	{0.f,   		1.f, 		 0.f, 		1.f, 		false, 	0.5f, 	"Amp Env Sustain"}},
	{params_e::amp_release,	{envTimingMin, envTimingMax, 0.f, 	envTimingSkew, 	false, 	0.8f, 	"Amp Env Release"}},

	{params_e::pos_scan_rate,   {-20.f, 20.f, 0.f,     0.3f, true,      0.f,    "Scanner Rate"}},
	{params_e::pos_scan_amount, {0.f,   1.f, 0.f,      1.f,  false,     0.f,    "Scanner Amount"}}
	
#ifdef TSN
	// 				   			min,   max,  spacing, skew, symmetrical, default, name
,	{params_e::nav_tendency_x,{-1.f,  1.f, 0.f, 		1.f, true, 		0.f, 	"Tendency X"}},
	{params_e::nav_tendency_y,{-1.f,  1.f, 0.f, 		1.f, true, 		0.f, 	"Tendency Y"}},
	{params_e::nav_tendency_z,{-1.f,  1.f, 0.f, 		1.f, true, 		0.f, 	"Tendency Z"}},
	{params_e::nav_tendency_u,{-1.f,  1.f, 0.f, 		1.f, true, 		0.f, 	"Tendency U"}},
	{params_e::nav_tendency_v,{-1.f,  1.f, 0.f, 		1.f, true, 		0.f, 	"Tendency V"}},
	{params_e::nav_tendency_w,{-1.f,  1.f, 0.f, 		1.f, true, 		0.f, 	"Tendency W"}},

	{params_e::nav_lfo_2d_amount, 	{0.f, 	1.f, 0.f, 	1.f, false, 	0.1f, 	"Amount"}},
	{params_e::nav_lfo_2d_rate, 	{0.1f, 10.f, 0.f, 	1.f, false, 	0.5f, 	"Rate"}},
	{params_e::nav_lfo_2d_response, {0.01f, 4.f, 0.f, 	1.f, false, 	0.01f, 	"Response"}},
	{params_e::nav_lfo_2d_overshoot, {0.55f, 24.f, 0.f, 0.5f, false, 	0.55f, 	"Overshoot"}},

	// 				   						min,   max,  spacing, skew, symmetrical, default, name

	{params_e::nav_random_walk_step_size, 	{0.f, 0.1f, 0.f,	1.f,	false,		0.01f,	"Walk Step Size"}}
#endif
};

static constexpr int NUM_MAIN_PARAMS = static_cast<int>(params_e::count_main_granular_params);
static constexpr int NUM_ENV_PARAMS = static_cast<int>(params_e::count_envelope_params) - static_cast<int>(params_e::count_main_granular_params) - 1;

static constexpr int NUM_SCANNER_PARAMS = static_cast<int>(params_e::count_pos_scan_params) - static_cast<int>(params_e::count_envelope_params) - 1;

static_assert(NUM_ENV_PARAMS == 4);
static_assert(NUM_SCANNER_PARAMS == 2);

#ifdef TSN
static constexpr int NUM_LFO2D_PARAMS = static_cast<int>(params_e::count_lfo_2d_params) - static_cast<int>(params_e::count_nav_tendency_params) - 1;
static_assert(NUM_LFO2D_PARAMS == 4);

static constexpr int NUM_RANDOM_WALK_PARAMS = static_cast<int>(params_e::count_random_walk_params) - static_cast<int>(params_e::count_lfo_2d_params) - 1;
static_assert(NUM_RANDOM_WALK_PARAMS == 1);
#endif


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
