/*
  ==============================================================================

    GranularSynthesis.cpp
    Created: 6 Aug 2023 12:44:38pm
    Author:  Nicholas Solem

  ==============================================================================
*/
/**
 TODO:
 -optimize:
	-it's not necessary to use some of the gen-translated functions, like switch,  gateSelect, or latch
	-polToCar calls std::sin and std::cos
 -CONSOLIDATE params into a struct that builds in gaussian randomizer
 */

#include "GranularSynthesis.h"
#include "../dsp_util.h"
#include "../algo_util.h"
#include <numbers>
#include <random>
#include <span>
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include "fmt/core.h"
#endif

#define pade false	// unfortunately, this optimization did not seem to clearly improve performance.

namespace nvs::gran {

template <typename float_t>
float semitonesToRatio(float_t semitones){
	constexpr float_t semitoneRatio = static_cast<float_t>(1.059463094359295);
	
#if(pade)
	float_t transpositionRatio = pow_fixed_base<float_t, semitoneRatio>(semitones);
#else
	float_t transpositionRatio = std::pow(semitoneRatio, semitones);
#endif
	
	return transpositionRatio;
}
inline float fastSemitonesToRatio(float semitones){
	return nvs::util::semitonesRatioTable(semitones);
}
namespace {
/**
 * Picks `numToPick` distinct bounds from `choices`, sampling each
 * with probability ∝ its `weight`.  If `numToPick` >= choices.size(),
 * returns all of them in arbitrary (but weighted) order.
 */
using WeightedReadBounds = PolyGrain::WeightedReadBounds;
std::vector<WeightedReadBounds> pickWeightedReadBoundsProbabilistically (std::vector<WeightedReadBounds> choices, int numToPick)
{
	const int N = (int) choices.size();
	if (N == 0 || numToPick <= 0){
		return {};
	}
	// 1) Extract weights into their own array:
	std::vector<double> weightArr;
	weightArr.reserve(N);
	for (auto& wrb : choices)
		weightArr.push_back(wrb.weight);

	std::mt19937 rng{ std::random_device{}() };
	std::vector<WeightedReadBounds> picked;
	picked.reserve(numToPick);

	if (numToPick <= N)
	{
		// --- WITHOUT replacement ---
		for (int pick = 0; pick < numToPick; ++pick)
		{
			std::discrete_distribution<int> dist(weightArr.begin(), weightArr.end());
			int choice = dist(rng);
			picked.emplace_back( choices[choice] );

			// zero out that weight, then renormalize
			weightArr[choice] = 0.0;
			double sum = std::accumulate(weightArr.begin(), weightArr.end(), 0.0);
			if (sum <= 0.0) {
				break;
			}
			for (auto& w : weightArr) {
				w /= sum;
			}
		}
	}
	else {
		// --- WITH replacement ---
		// we keep the original weightArr intact
		std::discrete_distribution<int> dist(weightArr.begin(), weightArr.end());
		for (int pick = 0; pick < numToPick; ++pick)
		{
			int choice = dist(rng);
			picked.push_back( choices[choice] );
			// note: weights are unchanged, so repeats are allowed
		}
	}

	return picked;
}
/**
 This version simply evenly distributes the choices amongst the available grains.
 */
std::vector<WeightedReadBounds> pickWeightedReadBoundsEvenly (std::vector<WeightedReadBounds> choices, int numToPick)
{
	
	const int N = (int) choices.size();

	jassert(N <= numToPick);

	if (N == 0 || numToPick <= 0){
		return {};
	}

	std::vector<WeightedReadBounds> picked;
	picked.reserve(numToPick);

	for (int i = 0; i < numToPick; ++i){
		picked.push_back(choices[i % N]);
	}

	return picked;
}
}
PolyGrain::PolyGrain(GranularSynthSharedState *const synth_shared_state,
					 GranularVoiceSharedState *const voice_shared_state)
:
_synth_shared_state { synth_shared_state },
_voice_shared_state(voice_shared_state),
_normalizer(1.f / std::sqrt(static_cast<float>(std::clamp(N_GRAINS, 1UL, 10000UL)))),
_grain_indices(N_GRAINS),
_speed_ler(_voice_shared_state->_expo_rng,  {10.f, 0.f})
{
	assert (_grains.size() == 0);
	_grains.reserve(N_GRAINS);
	for (size_t i = 0; i < N_GRAINS; ++i){
		_grains.emplace_back(_synth_shared_state, _voice_shared_state, i);
	}
	std::iota(_grain_indices.begin(), _grain_indices.end(), 0);
}
void PolyGrain::setSampleRate(double sample_rate){
	assert(_synth_shared_state);
	_phasor_internal_trig.setSampleRate(sample_rate);
	_voice_shared_state->_scanner.setSampleRate(sample_rate);
	_synth_shared_state->_playback_sample_rate = sample_rate;
}
void PolyGrain::setReadBounds(ReadBounds newReadBounds) {
	assert ((newReadBounds.begin >= 0.0) && (newReadBounds.begin <= 1.0));
	assert ((newReadBounds.end >= 0.0) && (newReadBounds.end <= 1.0));
	
	for (auto &g : _grains){
		// for now, we will just have all grains use same read bounds.
		// however, we may want to have some prorortions of grains using different readbounds in the future.
		g.setReadBounds(newReadBounds);
	}
}
void PolyGrain::setMultiReadBounds(std::vector<WeightedReadBounds> newWeightedReadBounds) {
	
	auto pickedWeightedReadBounds = pickWeightedReadBoundsEvenly(newWeightedReadBounds, (int)_grains.size());
	double w_sum = 0.0;
	for (auto wrb : pickedWeightedReadBounds){
		jassert(wrb.weight >= 0.0);
		w_sum += wrb.weight;
	}
	if (w_sum == 0.0) {
		w_sum = 1.0;
	}
	
	std::sort(pickedWeightedReadBounds.begin(), pickedWeightedReadBounds.end(), [](auto a, auto b) { return a.weight < b.weight; });
	for (size_t i = 0; i < _grains.size(); ++i){
		// for now, we will just have all grains use same read bounds.
		// however, we may want to have some prorortions of grains using different readbounds in the future.
		auto b = pickedWeightedReadBounds[i];
		assert ((b.bounds.begin >= 0.0) && (b.bounds.begin <= 1.0));
		assert ((b.bounds.end >= 0.0) && (b.bounds.end <= 1.0));
		
		_grains[i].setReadBounds(b.bounds);
		auto w = b.weight;
		w *= w;
		_grains[i].setWeight(w);
	}
}

void PolyGrain::setLogger(std::function<void(const juce::String&)> loggerFunction) {
	assert(_synth_shared_state);
	_synth_shared_state->_logger_func = loggerFunction;
}

void PolyGrain::doNoteOn(noteNumber_t note, velocity_t velocity){
	// reassign to noteHolder
	auto p = std::make_pair(note, velocity);

	_note_holder.insert(p);
	updateNotes();
	_phasor_internal_trig.reset();
	_voice_shared_state->_scanner.reset();
	
#if GRAIN_UPDATE_HACK
	for (int i = 0; i < _grains.size(); ++i) {
		_grains[i].setFirstPlaythroughOfVoicesNote(true);
	}
#endif
}
void PolyGrain::doNoteOff(noteNumber_t note){
	// remove from noteHolder
//	auto p = std::make_pair(note, 0);
	_note_holder[note] = 0;
	updateNotes();
	_note_holder.erase(note);
	updateNotes();
}
void PolyGrain::doUpdateNotes(){
	size_t num_notes = _note_holder.size();
	float grainsPerNoteFloor = N_GRAINS / static_cast<float>(num_notes);

	auto begin = _grains.begin();
	float fractional_right_side = 0.f;
	for (auto e : _note_holder){
		auto left = begin + static_cast<size_t>(fractional_right_side);
		fractional_right_side += grainsPerNoteFloor;
		auto right = begin + static_cast<size_t>(fractional_right_side);
		for (; left != right; ++left){
			float note = e.first;
			float rat = fastSemitonesToRatio(note - 69);
			(*left).setRatioBasedOnNote(rat);
			float vel = e.second;
			float amp = vel / static_cast<float>(100);
			(*left).setAmplitudeBasedOnNote(amp);
		}
	}
}
void PolyGrain::doClearNotes(){
	_note_holder.clear();
}
void PolyGrain::doShuffleIndices(){
	std::shuffle(_grain_indices.begin(), _grain_indices.end(),
				 _voice_shared_state->_gaussian_rng.getGenerator());
}
std::vector<float> PolyGrain::getBusyStatuses() const {
	std::vector<float> busyStatuses;
	busyStatuses.reserve(N_GRAINS);
	for (auto const &g : _grains){
		busyStatuses.push_back(g.getBusyStatus());
	}
	return busyStatuses;
}
void PolyGrain::setGrainsIdle() {
	for (auto &g : _grains){
		g.setBusyStatus(false);
	}
}

void PolyGrain::setParams() {
	auto const &apvts = _synth_shared_state->_apvts;
	_speed_ler.setMu(*apvts.getRawParameterValue("speed"));
	_speed_ler.setSigma(*apvts.getRawParameterValue("speed_rand"));
	_voice_shared_state->_scanner._freq = *apvts.getRawParameterValue("scanner_rate");
	_voice_shared_state->_scanner_amount = *apvts.getRawParameterValue("scanner_amount");
	
	for (auto &g : _grains){
		g.setParams();
	}
}
std::array<float, 2> PolyGrain::doProcess(float trigger_in){
	std::array<float, 2> output {0.f, 0.f};
	
	// update phasor's frequency only if _triggerHisto.val is true
	float const freq_tmp = _speed_ler(static_cast<bool>(_trigger_histo.val)); // used to clamp by percentage of mu. should no longer be necessary.
	_phasor_internal_trig.setFrequency(freq_tmp);
	++_phasor_internal_trig;
	float trig = _ramp2trig(_phasor_internal_trig.getPhase());
	_trigger_histo(trig);
	trig = (!trig && !trigger_in) ? 0.f : 1.f;
	
	_voice_shared_state->_scanner.phasor();	// increment scanner phase per sample

	std::array<Grain::outs, N_GRAINS> _outs;

	size_t idx = _grain_indices[0];
	_outs[idx] = _grains[idx](trig);
	float audio_out_L = _outs[idx].audio_L;
	float audio_out_R = _outs[idx].audio_R;
	float voices_active = _outs[idx].busy;
	
	for (size_t i = 1; i < N_GRAINS; ++i){
		idx = _grain_indices.data()[i];
		size_t prevIdx = _grain_indices.data()[i - 1];
		
		float currentTrig = _outs[prevIdx].next;
		_outs[idx] = _grains[idx](currentTrig);
		audio_out_L += _outs[idx].audio_L;
		audio_out_R += _outs[idx].audio_R;
		voices_active += _outs[idx].busy;
	}
	output[0] = audio_out_L * _normalizer;
	output[1] = audio_out_R * _normalizer;

	if (nvs::util::checkNanOrInf(output)){
		return {};
	}
	return output;
}

std::vector<GrainDescription> PolyGrain::getGrainDescriptions() const {
	std::vector<GrainDescription> gds(N_GRAINS);
	for (size_t i = 0; i < N_GRAINS; ++i){
		gds[i] = _grains[i].getGrainDescription();
	}
	assert(gds.size() == N_GRAINS);
	return gds;
}

//=====================================================================================
void Grain::setParams(){
	auto const &apvts = _synth_shared_state->_apvts;
	_transpose_lgr.setMu(*apvts.getRawParameterValue("transpose"));
	_transpose_lgr.setSigma(24.0f * (*apvts.getRawParameterValue("transpose_rand")));
	_duration_ler.setMu(*apvts.getRawParameterValue("duration"));
	_duration_ler.setSigma(*apvts.getRawParameterValue("duration_rand"));
	float const pos = *apvts.getRawParameterValue("position");
	_position_lgr.setMu(pos);
	_position_lgr.setSigma(*apvts.getRawParameterValue("position_rand"));
	_skew_lgr.setMu(*apvts.getRawParameterValue("skew"));
	_skew_lgr.setSigma(*apvts.getRawParameterValue("skew_rand"));
	_plateau_lgr.setMu(*apvts.getRawParameterValue("plateau"));
	_plateau_lgr.setSigma(*apvts.getRawParameterValue("plateau_rand"));
	_pan_lgr.setMu(1.f - *apvts.getRawParameterValue("pan"));	// makes more sense internally to reverse this
	_pan_lgr.setSigma(*apvts.getRawParameterValue("pan_rand"));
	
	_grain_drive = *apvts.getRawParameterValue("fx_grain_drive");
	_grain_makeup_gain = *apvts.getRawParameterValue("fx_makeup_gain");
}

Grain::Grain(GranularSynthSharedState *const synth_shared_state,
					 GranularVoiceSharedState *const voice_shared_state,
					 int newId)
:	_synth_shared_state(synth_shared_state)
,	_voice_shared_state(voice_shared_state)
,	_grain_id(newId)
,	_transpose_lgr(_voice_shared_state->_gaussian_rng, {0.f, 0.f})
,	_position_lgr(_voice_shared_state->_gaussian_rng, {0.0, 0.0})
,	_duration_ler(_voice_shared_state->_expo_rng, {0.015f, 0.f})
,	_skew_lgr(_voice_shared_state->_gaussian_rng, {0.5f, 0.f})
,	_plateau_lgr(_voice_shared_state->_gaussian_rng, {1.f, 0.f})
,	_pan_lgr(_voice_shared_state->_gaussian_rng, {0.5f, 0.23f})
{}
void Grain::setRatioBasedOnNote(float ratioForNote){
	_ratio_based_on_note = ratioForNote;
}
void Grain::setAmplitudeBasedOnNote(float velocity){
	assert(velocity <= 2.f);
	assert(velocity >= 0.f);
	_amplitude_based_on_note = velocity;
}
void Grain::setId(int newId){
	_grain_id = newId;
}

float Grain::getBusyStatus() const {
	return _busy_histo.val;
}
void Grain::setBusyStatus(bool newBusyStatus) {
	_busy_histo.val = static_cast<float>(newBusyStatus);
}

GrainDescription Grain::getGrainDescription() const {
	assert(_synth_shared_state);
	auto const N = _synth_shared_state->_buffer._wave_block.getNumSamples();

	GrainDescription gd;
	gd.voice = _voice_shared_state->_voice_id;
	gd.grain_id = _grain_id;
	gd.position = nvs::gen::wrap01(_sample_index / N);
	gd.sample_playback_rate = _waveform_read_rate;
	gd.window = _window;
	gd.pan = _pan / (std::numbers::pi * 0.5f);
	gd.busy = _busy_histo.val != 0.f;
#if GRAIN_UPDATE_HACK
	gd.first_playthrough = firstPlaythroughOfVoicesNote;
#else
	gd.first_playthrough = false;
#endif
	return gd;
}

namespace {	// anonymous namespace for local helper functions
float calculateTransposeMultiplier(float const ratioBasedOnNote, float const ratioBasedOnTranspose){
	return memoryless::clamp(ratioBasedOnNote * ratioBasedOnTranspose, 0.001f, 1000.f);
}
double calculateDurationInSamples(double latchedDuration, double compensatedLength, double sampleRate){
	using nvs::memoryless::clamp;
#pragma message("this clamping of randomized duration should be improved")
	auto const clippedNormalizedDuration = clamp(latchedDuration, 0.0, 1.0);
	double constexpr maxLengthInSeconds = 20.0;
	double constexpr minLengthInSeconds = 0.001;	// 1 ms
	double const maxLengthInSamples = maxLengthInSeconds * sampleRate;
	double const minLengthInSamples = minLengthInSeconds * sampleRate;
//	auto const clippedLength = clamp(compensatedLength, minLengthInSamples, maxLengthInSamples);
	return clamp(clippedNormalizedDuration * compensatedLength, minLengthInSamples, maxLengthInSamples);
}
float calculateWindow(double const accum, double const duration, float const transpositionMultiplier, float const skew, float plateau){
	assert(transpositionMultiplier > 0.f);
	assert (duration > 0.0);
	double const v = (accum / duration);
	double const windowIdx = nvs::memoryless::clamp(v / transpositionMultiplier, 0.0, 1.0);
	float win = nvs::gen::triangle<float, false>(static_cast<float>(windowIdx), skew);
	
	plateau = memoryless::clamp_low(plateau, 0.000001f);
	win *= plateau;
#define tanh nvs::util::tanh_pade_3_2
	win = gen::parzen(tanh(win)) / gen::parzen(tanh(plateau));
	if (plateau < 1.0){
#if pade
		win = nvs::util::pow_pade(win, 1.f / plateau);
#else
		win = pow(win, 1.f / plateau);
#endif
	}
	return win;
}
double calculateSampleReadRate(double const playback_sample_rate, double const file_sample_rate){
	assert(playback_sample_rate > 0.0);
	assert(file_sample_rate > 0.0);
	return file_sample_rate / playback_sample_rate;
}
float getDurationPitchCompensationFactor(float const duration_pitch_compensation_amount, float waveform_read_rate){
	assert (duration_pitch_compensation_amount >= 0.f);
	assert (duration_pitch_compensation_amount <= 1.f);
	return waveform_read_rate * duration_pitch_compensation_amount + (1.f - duration_pitch_compensation_amount);
}
double calculateCenterOfEnvelope(double const normalized_position, double const sr_compensated_duration, float const skew,
								 float const sample_playback_rate, bool const center_envelope_at_env_peak)
{
	double const center_of_env = center_envelope_at_env_peak ?
		skew * sr_compensated_duration * sample_playback_rate
		: sr_compensated_duration * normalized_position;
	return center_of_env;
}
double calculateSampleIndex(double const accum,
							double const normalized_position,
							double const sample_left_bound,
							double const sample_right_bound,
							double const sample_rate_compensate_ratio,
							double const center_of_env)
{
	double const position_in_samps = sample_left_bound + normalized_position * (sample_right_bound - sample_left_bound);
	double const sample_index = sample_rate_compensate_ratio * (accum - center_of_env) + position_in_samps;
	return sample_index;
}
float calculateSample(juce::dsp::AudioBlock<float> const wave_block, double const sample_index, float const win, float const velocity_amplitude){
	assert(wave_block.getNumChannels() > 0);
	assert(wave_block.getNumSamples() > 0);
	auto const samp = gen::peek<float,
						gen::interpolationModes_e::hermite,
						gen::boundsModes_e::wrap
						>(wave_block.getChannelPointer(0), sample_index, wave_block.getNumSamples());
	return win * velocity_amplitude * samp;
}
float calculatePan(float pan_latch_val){
	return memoryless::clamp(pan_latch_val, 0.f, 1.f) * std::numbers::pi * 0.5f;
}
void writeAudioToOuts(float const sample, float const pan_latch_val, GrainwisePostProcessing &postProcessing, Grain::outs &outs){
	std::array<float, 2> const lr = postProcessing(gen::pol2car(sample, pan_latch_val));
	outs.audio_L = lr[0];
	outs.audio_R = lr[1];
}
void processBusyness(float const window, nvs::gen::history<float> &busyHistory, Grain::outs &outs){
	float const  busy_tmp = (window > 0.f);
	outs.busy = busy_tmp;
	busyHistory(busy_tmp);
}
}	// end anonymous namespace

void Grain::setReadBounds(ReadBounds newReadBounds){
	_upcoming_normalized_read_bounds = newReadBounds;
}
void Grain::resetAccum() {
	_accum.reset();
}
void Grain::setAccum(float newVal) {
	_accum.set(newVal);
}
float GrainwisePostProcessing::operator()(float x){
	float retval {0.f};
	jassert (drive > 0);
	x *= drive;
	retval = (2.f * x) / (1.f + std::sqrt(1.f + std::abs(x)));
	retval /= (2.f * drive) / (1.f + std::sqrt(1.f + std::abs(drive)));
	jassert(makeup_gain > 0.f);
	retval *= makeup_gain;
	return retval;
}
Grain::outs Grain::operator()(float const trig_in){
	assert(_synth_shared_state);
	juce::dsp::AudioBlock<float> const wave_block = _synth_shared_state->_buffer._wave_block;
	auto const playback_sr = _synth_shared_state->_playback_sample_rate;
	auto const file_sr = _synth_shared_state->_buffer._file_sample_rate;
	auto const& settings = _synth_shared_state->_settings;
	
	outs o;
	o.next = _busy_histo.val ? trig_in : 0.f;
	
	bool const should_open_latches = _busy_histo.val ? false : static_cast<bool>(trig_in);

	
	_waveform_read_rate = calculateTransposeMultiplier(_ratio_for_note_latch(_ratio_based_on_note, should_open_latches),
													   fastSemitonesToRatio(_transpose_lgr(should_open_latches)));
	_accum(_waveform_read_rate, static_cast<bool>(should_open_latches));
	
	double const file_sample_rate_compensate_ratio = calculateSampleReadRate(playback_sr, file_sr);

	if (should_open_latches){
		_normalized_read_bounds = _upcoming_normalized_read_bounds;
		_postProcessing.drive = _grain_drive;
		_postProcessing.makeup_gain = _grain_makeup_gain;

#if GRAIN_UPDATE_HACK
		if (wantsToDisableFirstPlaythroughOfVoicesNote){
			firstPlaythroughOfVoicesNote = false;
			wantsToDisableFirstPlaythroughOfVoicesNote = false;
		}
		wantsToDisableFirstPlaythroughOfVoicesNote = true;
#endif
	}
	
	
	if (_normalized_read_bounds.end - _normalized_read_bounds.begin == 0.0){	// protection for initialization case
		_window = 0.f;
		writeAudioToOuts(0.f, 0.f, _postProcessing, o);
		processBusyness(_window, _busy_histo, o);
		return o;
	}
	
	auto const buffLength = _synth_shared_state->_buffer._wave_block.getNumSamples();
	ReadBounds denormedReadBounds = _normalized_read_bounds * static_cast<double>(buffLength);
	if (denormedReadBounds.end < denormedReadBounds.begin){
		denormedReadBounds.end += buffLength;	// now this can be longer than the actual number of samples in the buffer. should be taken care of by wrapping in peek().
		assert (denormedReadBounds.end > denormedReadBounds.begin);
	}

	size_t const compensatedLength = [&settings, &denormedReadBounds, buffLength, file_sample_rate_compensate_ratio](){
		float const dur_dep_on_read_bounds = settings._duration_dependence_on_read_bounds;
		size_t const event_length = denormedReadBounds.end - denormedReadBounds.begin;
		size_t const cLen = static_cast<size_t>(nvs::memoryless::linterp((float)buffLength, (float)event_length, dur_dep_on_read_bounds) / file_sample_rate_compensate_ratio);
		assert (0 < cLen);
		return cLen;
	}();

	double const duration_in_samps = calculateDurationInSamples(_duration_ler(should_open_latches), compensatedLength,
																playback_sr);	// take settings._center_position_at_env_peak as param to determine if it should clip normalized duration to 0-1?
	assert (duration_in_samps <= compensatedLength);
	float const latch_skew_result = memoryless::clamp(_skew_lgr(should_open_latches), 0.001f, 0.999f);
	
	float const duration_pitch_compensation_factor = getDurationPitchCompensationFactor(settings._duration_pitch_compensation, _waveform_read_rate);

	double norm_pos = [this, should_open_latches](){
		double np = _position_lgr(should_open_latches);
		auto const scanner_pos = _scanner_for_position_latch(_voice_shared_state->_scanner.phasor_offset(0.f) * _voice_shared_state->_scanner_amount, should_open_latches);
		np = nvs::memoryless::mspWrap(np + scanner_pos);
		assert (np >= 0.0);
		assert (np <= 1.0);
		return np;
	}();
	
	_window = calculateWindow(_accum.val,							// double const accum
							  duration_in_samps,					// double const duration
							  duration_pitch_compensation_factor,	// float const transpositionMultiplier
							  latch_skew_result,					// float const skew
							  _plateau_lgr(should_open_latches));	// float plateau
#if GRAIN_UPDATE_HACK
	if (firstPlaythroughOfVoicesNote){
		writeAudioToOuts(0.f, 0.f, o);
		processBusyness(_window, _busy_histo, o);
		return o;
	}
#endif

	_sample_index = [this, norm_pos, duration_in_samps, latch_skew_result, duration_pitch_compensation_factor, file_sample_rate_compensate_ratio, &settings, &denormedReadBounds](){
		// double const normalized_position, double const sr_compensated_duration, float const skew, float const sample_playback_rate, bool const center_envelope_at_env_peak
		auto const center_of_env = calculateCenterOfEnvelope(norm_pos,														// double const normalized_position
															 duration_in_samps,												// double const sr_compensated_duration
															 latch_skew_result,												// float const skew
															 duration_pitch_compensation_factor,							// float const sample_playback_rate
															 settings._center_position_at_env_peak);	// bool const center_envelope_at_env_peak
				
		return calculateSampleIndex(_accum.val,							// double const accum
							 norm_pos,									// double const normalized_position
							 denormedReadBounds.begin, 					// double const sample_left_bound
							 denormedReadBounds.end,					// double const sample_right_bound
							 file_sample_rate_compensate_ratio,			// double const sample_rate_compensate_ratio
							 center_of_env);							// double const center_of_env
	}();
	

	float const sample = [&](){
		float const vel_amplitude = _amplitude_for_note_latch(_amplitude_based_on_note, should_open_latches)
#ifdef TSN
								* _grain_weight_latch(_grain_weight, should_open_latches);
#endif
		;
		return calculateSample(wave_block, _sample_index, _window, vel_amplitude);
	}();
	_pan = calculatePan(_pan_lgr(should_open_latches));
	
	writeAudioToOuts(sample, _pan, _postProcessing, o);
	
	processBusyness(_window, _busy_histo, o);
	
	if (nvs::util::checkNanOrInf(std::array<float, 2>{o.audio_L, o.audio_R})){
		return {};
	}
	
	return o;
}
}	// namespace nvs::gran
