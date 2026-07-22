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
	-setParams should not need to be called in every PolyGrain (a.k.a. per-voice), as it is getting params global to the synth
 -CONSOLIDATE params into a struct that builds in gaussian randomizer
 */

#include "GranularSynthesis.h"
#include "../utils/dsp_util.h"
#include "../utils/algo_util.h"
#include <numbers>
#include <random>
#include <span>

#include "StringAxiom.h"
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include "fmt/core.h"
#endif

#define pade false	// unfortunately, this optimization did not seem to clearly improve performance.

namespace nvs::gran {
// NOLINTBEGIN(cppcoreguidelines-narrowing-conversions)

namespace {

using WeightedReadBounds = PolyGrain::WeightedReadBounds;

/**
 This version simply evenly distributes the choices amongst the available grains.
 */
struct WeightedReadBoundsAndFrequency {
    WeightedReadBounds wb;
    float freq {0.f};
};
std::vector<WeightedReadBoundsAndFrequency> pickWeightedReadBoundsEvenly (const std::vector<WeightedReadBounds> &wrb, const std::array<float, 3> fundamentals, const int numToPick)
{
	
	constexpr int N = fundamentals.size();
    jassert(wrb.size() == N);

	jassert(N <= numToPick);

	if (numToPick <= 0){
		return {};
	}

	std::vector<WeightedReadBoundsAndFrequency> picked;
	picked.reserve(numToPick);

	for (int i = 0; i < numToPick; ++i){
		picked.push_back(WeightedReadBoundsAndFrequency {
		    .wb = wrb[i % N],
		    .freq = fundamentals[i % N]
		});
	}

	return picked;
}
[[maybe_unused]] std::vector<WeightedReadBounds> pickWeightedReadBoundsProbabilistically (const std::vector<WeightedReadBounds> &choices, const int numToPick)
	/**
	 * Picks `numToPick` distinct bounds from `choices`, sampling each
	 * with probability ∝ its `weight`.  If `numToPick` >= choices.size(),
	 * returns all of them in arbitrary (but weighted) order.
	 */
{
    const int N = static_cast<int>(choices.size());
    if (N == 0 || numToPick <= 0){
	    return {};
    }
    // 1) Extract weights into their own array:
    std::vector<double> weightArr;
    weightArr.reserve(N);
    for (auto& wrb : choices) {
	    weightArr.push_back(wrb.weight);
    }

    std::mt19937 rng{ std::random_device{}() };
    std::vector<WeightedReadBounds> picked;
    picked.reserve(numToPick);

    if (numToPick <= N)
    {
	    // --- WITHOUT replacement ---
	    for (int pick = 0; pick < numToPick; ++pick)
	    {
		    std::discrete_distribution dist(weightArr.begin(), weightArr.end());
		    int choice = dist(rng);
		    picked.emplace_back( choices[choice] );

		    // zero out that weight, then renormalize
		    weightArr[choice] = 0.0;
		    const double sum = std::accumulate(weightArr.begin(), weightArr.end(), 0.0);
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
	    std::discrete_distribution dist(weightArr.begin(), weightArr.end());
	    for (int pick = 0; pick < numToPick; ++pick)
	    {
		    const int choice = dist(rng);
		    picked.push_back( choices[choice] );
		    // note: weights are unchanged, so repeats are allowed
	    }
    }

    return picked;
}

}   // anonymous namespace

PolyGrain::PolyGrain(GranularSynthSharedState *const synth_shared_state,
					 GranularVoiceSharedState *const voice_shared_state)
:
_synth_shared_state { synth_shared_state },
_voice_shared_state(voice_shared_state),
_normalizer(1.f / std::sqrt(static_cast<float>(std::clamp(N_GRAINS, 1UL, 10000UL)))),
_grain_indices(N_GRAINS),
_speed_lnr(_voice_shared_state->_gaussian_rng,  {1.0, 0.0})
{
	assert (_grains.empty());
	_grains.reserve(N_GRAINS);
	for (size_t i = 0; i < N_GRAINS; ++i){
		_grains.emplace_back(_synth_shared_state, _voice_shared_state, i);
	}
	std::iota(_grain_indices.begin(), _grain_indices.end(), 0);
}
void PolyGrain::setSampleRate(const double sampleRate){
	assert(_synth_shared_state);
	_phasor_internal_trig.setSampleRate(sampleRate);
	_voice_shared_state->_scanner.lfo.setSampleRate(sampleRate);
	_synth_shared_state->_playback_sample_rate = sampleRate;
}
void PolyGrain::setReadBounds(const ReadBounds newReadBounds) {
	assert (newReadBounds.begin >= 0.0 && newReadBounds.begin <= 1.0);
	assert (newReadBounds.end >= 0.0 && newReadBounds.end <= 1.0);
	
	for (auto &g : _grains){
		// for now, we will just have all grains use same read bounds.
		// however, we may want to have some proportions of grains using different readbounds in the future.
		g.setReadBounds(newReadBounds);
	}
}

#pragma message("needs more benchmarking")
float sqrtCached(const float x) {
    // this is useful because many incoming weights should be identical
    static constexpr size_t CACHE_SIZE = 16;
    static std::array<std::pair<float, float>, CACHE_SIZE> cache{};
    static size_t next_slot = 0;

    for (const auto& [key, val] : cache) {
        if (key == x) return val;
    }

    float answer = std::sqrt(x);
    cache[next_slot] = {x, answer};
    next_slot = (next_slot + 1) % CACHE_SIZE;

    return answer;
}
void PolyGrain::setEvents(const std::vector<WeightedReadBounds> &newWeightedReadBounds, const std::array<float, 3> &fundamental_frequencies) {
	auto picked = pickWeightedReadBoundsEvenly(newWeightedReadBounds, fundamental_frequencies, static_cast<int>(_grains.size()));

	std::ranges::sort(picked, [](auto a, auto b) { return a.wb.weight < b.wb.weight; });
	for (size_t i = 0; i < _grains.size(); ++i){
		// for now, we will just have all grains use same read bounds.
		// however, we may want to have some proportions of grains using different readbounds in the future.
		const auto &[wb, freq] = picked[i];
        assert (wb.bounds.begin >= 0.0 && wb.bounds.begin <= 1.0);
        assert ( wb.bounds.end  >= 0.0 &&  wb.bounds.end  <= 1.0);


		_grains[i].setReadBounds(wb.bounds);
		auto w = wb.weight;
		w *= w;
		_grains[i].setWeight(sqrtCached(w));
	    _grains[i].setUnderlyingFundamentalFrequency(freq);
	}
}

void PolyGrain::setLogger(const std::function<void(const String&)> &loggerFunction) const {
	assert(_synth_shared_state);
	_synth_shared_state->_logger_func = loggerFunction;
}

void PolyGrain::doNoteOn(noteNumber_t note, velocity_t velocity){
	// reassign to noteHolder
	auto p = std::make_pair(note, velocity);

	_note_holder.insert(p);
	updateNotes();
	_phasor_internal_trig.reset();
	_voice_shared_state->_scanner.lfo.reset();
}
void PolyGrain::doNoteOff(const noteNumber_t note){
	// remove from noteHolder
	// _note_holder[note] = 0;
	// updateNotes();
	_note_holder.erase(note);
	updateNotes();
}
void PolyGrain::doUpdateNotes(){
	const size_t num_notes = _note_holder.size();
	const float grainsPerNoteFloor = N_GRAINS / static_cast<float>(num_notes);

	const auto begin = _grains.begin();
	float fractional_right_side = 0.f;
	for (auto [note, vel] : _note_holder){
		auto left = begin + static_cast<size_t>(fractional_right_side);
		fractional_right_side += grainsPerNoteFloor;
		auto right = begin + static_cast<size_t>(fractional_right_side);
		for (; left != right; ++left){
			const float rat = util::midiToFrequency(note-69.f,
			    _synth_shared_state->_concertPitchHz,
			    69.f,
			    _synth_shared_state->_notesPerOctave);
			(*left).setRatioBasedOnNote(rat);
			const float amp = vel / static_cast<float>(100);
			(*left).setAmplitudeBasedOnNote(amp);
		}
	}
}
void PolyGrain::doClearNotes(){
	_note_holder.clear();
}
void PolyGrain::doShuffleIndices(){
	std::ranges::shuffle(_grain_indices,
                         _voice_shared_state->_gaussian_rng.getGenerator());
}
std::vector<float> PolyGrain::getBusyStatuses() const {
	std::vector<float> busyStatuses;
	busyStatuses.reserve(N_GRAINS);
	for (const auto &g : _grains){
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
	const auto &apvts = _synth_shared_state->_apvts;
	_speed_lnr.setMu(*apvts.getRawParameterValue("speed"));
	_speed_lnr.setSigma(*apvts.getRawParameterValue("speed_rand"));

	_voice_shared_state->_scanner.lfo._freq = *apvts.getRawParameterValue("scanner_rate");
	_voice_shared_state->_scanner.amount = *apvts.getRawParameterValue("scanner_amount");
    _voice_shared_state->_scanner.shape = *apvts.getRawParameterValue("scanner_shape");
	
	for (auto &g : _grains){
		g.setParams();
	}
}

std::array<float, 2> PolyGrain::doProcess(const float triggerIn){
	std::array output {0.f, 0.f};
	
	// update phasor's frequency only if _triggerHisto.val is true
    const bool should_open_latches = static_cast<bool>(_trigger_histo.val) || _voice_shared_state->forceGrainTrigger;

	_voice_shared_state->grain_rate_hz = _speed_lnr(should_open_latches); // used to clamp by percentage of mu. should no longer be necessary.
	_phasor_internal_trig.setFrequency(_voice_shared_state->grain_rate_hz);
	++_phasor_internal_trig;
	float trig = _ramp2trig(_phasor_internal_trig.getPhase());
	_trigger_histo(trig);
	trig = !trig && !triggerIn ? 0.f : 1.f;
	
	_voice_shared_state->_scanner.lfo.phasor();	// increment scanner phase per sample

	std::array<Grain::outs, N_GRAINS> _outs;

#pragma message("trying shuffle, may cause bugs (so far seems fine but keeping warning here until i can be completely sure)")
	shuffleIndices();   // since grains are assigned read bounds deterministically and relatively rarely, shuffle is needed to get a thorough mix of effective read position
	
	size_t idx = _grain_indices[0];
	_outs[idx] = _grains[idx](trig);
	float audio_out_L = _outs[idx].audio_L;
	float audio_out_R = _outs[idx].audio_R;
	float voices_active = _outs[idx].busy;

	for (size_t i = 1; i < N_GRAINS; ++i){
		idx = _grain_indices[i];
		const size_t prevIdx = _grain_indices[i - 1];

		const float currentTrig = _outs[prevIdx].next;
		_outs[idx] = _grains[idx](currentTrig);
		audio_out_L += _outs[idx].audio_L;
		audio_out_R += _outs[idx].audio_R;
		voices_active += _outs[idx].busy;
	}
    _voice_shared_state->forceGrainTrigger = false;

	output[0] = audio_out_L * _normalizer;
	output[1] = audio_out_R * _normalizer;

	if (util::checkNanOrInf(output)){
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
	const auto &apvts = _synth_shared_state->_apvts;
    const auto getExtent = [&apvts](StringRef paramID) {
        juce::NormalisableRange<float> skewRange = apvts.getParameterRange(paramID);
        const auto r = skewRange.getRange();
        return std::abs(r.getEnd() - r.getStart());
    };

	_transpose_lgr.setMu(*apvts.getRawParameterValue("transpose"));
    _frequencyRandomizationMode = *apvts.getRawParameterValue(axiom::frequency_randomization_mode)
        == 0.f ? FrequencyRandomizationMode::Continuous : FrequencyRandomizationMode::Octaves;
	_transpose_lgr.setSigma(*apvts.getRawParameterValue("transpose_rand") * 24.0f);
	_density_lnr.setMu(*apvts.getRawParameterValue("density"));
	_density_lnr.setSigma(*apvts.getRawParameterValue("density_rand"));
	_position_lgr.setMu(*apvts.getRawParameterValue("position"));
	_position_lgr.setSigma(*apvts.getRawParameterValue("position_rand"));
	_skew_lgr.setMu(*apvts.getRawParameterValue("skew"));
	_skew_lgr.setSigma(*apvts.getRawParameterValue("skew_rand") * getExtent("skew"));
	_plateau_lgr.setMu(*apvts.getRawParameterValue("plateau"));
	_plateau_lgr.setSigma(*apvts.getRawParameterValue("plateau_rand") * getExtent("plateau"));
	_pan_lgr.setMu(1.f - *apvts.getRawParameterValue("pan"));	// makes more sense internally to reverse this
	_pan_lgr.setSigma(*apvts.getRawParameterValue("pan_rand"));

    _pitchify = static_cast<bool>(*apvts.getRawParameterValue("pitchify"));

    //==============================================================================
    _grain_normalize_amount = *apvts.getRawParameterValue("fx_grain_normalize");

	_postProcessing.setDriveMu(*apvts.getRawParameterValue("fx_grain_drive"));
	_postProcessing.setDriveSigma(*apvts.getRawParameterValue("fx_grain_drive_rand"));

	_grain_makeup_gain = *apvts.getRawParameterValue("fx_makeup_gain");

	_postProcessing.setFilterCutoffMu(*apvts.getRawParameterValue("fx_filter_cutoff"));
	_postProcessing.setFilterCutoffSigma(*apvts.getRawParameterValue("fx_filter_cutoff_rand"));
	_postProcessing.setFilterQMu(*apvts.getRawParameterValue("fx_filter_q"));
	_postProcessing.setFilterQSigma(*apvts.getRawParameterValue("fx_filter_q_rand"));
	_postProcessing.setFilterModeMu(*apvts.getRawParameterValue("fx_filter_mode"));
	_postProcessing.setFilterModeSigma(*apvts.getRawParameterValue("fx_filter_mode_rand"));
}

Grain::Grain(GranularSynthSharedState *const synth_shared_state,
					 GranularVoiceSharedState *const voice_shared_state,
					 const int newId):
    _synth_shared_state(synth_shared_state)
    , _voice_shared_state(voice_shared_state)
    , _grain_id(newId)
    , _transpose_lgr(_voice_shared_state->_gaussian_rng, {0.f, 0.f})
    , _position_lgr(_voice_shared_state->_gaussian_rng, {0.0, 0.0})
    , _density_lnr(_voice_shared_state->_gaussian_rng, {0.5f, 0.f})
    , _skew_lgr(_voice_shared_state->_gaussian_rng, {0.0f, 0.f})
    , _plateau_lgr(_voice_shared_state->_gaussian_rng, {1.f, 0.f})
    , _pan_lgr(_voice_shared_state->_gaussian_rng, {0.5f, 0.23f})
    , _postProcessing(_synth_shared_state, _voice_shared_state)
    , _frequencyRandomizationMode()
    , _grainWindow(1.0, 1.f, 0.f, 0.f)
{
// #ifdef DBG
//     _timed_printer = std::make_unique<util::TimedPrinter>(100);
// #endif
}

void Grain::setRatioBasedOnNote(const float ratioForNote){
	_ratio_based_on_note = ratioForNote;
}
void Grain::setAmplitudeBasedOnNote(const float velocity){
	assert(velocity <= 2.f);
	assert(velocity >= 0.f);
	_amplitude_based_on_note = velocity;
}
void Grain::setId(const int newId){
	_grain_id = newId;
}

float Grain::getBusyStatus() const {
	return _busy_histo.val;
}
void Grain::setBusyStatus(const bool newBusyStatus) {
	_busy_histo.val = static_cast<float>(newBusyStatus);
}

GrainDescription Grain::getGrainDescription() const {
	assert(_synth_shared_state);
	const auto N = _synth_shared_state->_buffer._wave_block.getNumSamples();

	GrainDescription gd{};
	gd.voice = _voice_shared_state->_voice_id;
	gd.grain_id = _grain_id;
	gd.position = gen::wrap01(_sample_index / N);
	gd.sample_playback_rate = _waveform_read_rate;
	gd.window = _window_val;
	gd.pan = _pan / (std::numbers::pi * 0.5f);
	gd.busy = _busy_histo.val != 0.f;
	return gd;
}

namespace {	// anonymous namespace for local helper functions
float calculateTransposeMultiplier(const float ratioBasedOnNote, const float ratioBasedOnTranspose, const float ratioBasedOnUnderlyingF0){
	return memoryless::clamp(ratioBasedOnNote * ratioBasedOnTranspose * ratioBasedOnUnderlyingF0, 0.001f, 1000.f);
}

double calculateSampleReadRate(const double playback_sample_rate, const double file_sample_rate){
	assert(playback_sample_rate > 0.0);
	assert(file_sample_rate > 0.0);
	return file_sample_rate / playback_sample_rate;
}
float getDurationPitchCompensationFactor(const float duration_pitch_compensation_amount, const float waveform_read_rate){
	assert (duration_pitch_compensation_amount >= 0.f);
	assert (duration_pitch_compensation_amount <= 1.f);
	return waveform_read_rate * duration_pitch_compensation_amount + (1.f - duration_pitch_compensation_amount);
}
double calculateCenterOfEnvelope(const double normalized_position, const double sr_compensated_duration, const float skew,
								 const float sample_playback_rate, const bool center_envelope_at_env_peak)
{
	const double center_of_env = center_envelope_at_env_peak ?
		skew * sr_compensated_duration * sample_playback_rate
		: sr_compensated_duration * normalized_position;
	return center_of_env;
}
double calculateSampleIndex(const double accum,
							const double normalized_position,
							const double sample_left_bound,
							const double sample_right_bound,
							const double sample_rate_compensate_ratio,
							const double center_of_env)
{
	const double position_in_samps = sample_left_bound + normalized_position * (sample_right_bound - sample_left_bound);
	const double sample_index = sample_rate_compensate_ratio * (accum - center_of_env) + position_in_samps;
	return sample_index;
}
float calculateSample(const dsp::AudioBlock<float> &wave_block, const double sample_index,
    const float win,
    const float velocity_amplitude)
{
	assert(wave_block.getNumChannels() > 0);
	assert(wave_block.getNumSamples() > 0);
	const auto samp = gen::peek<float,
						gen::interpolationModes_e::hermite,
						gen::boundsModes_e::wrap
						>(wave_block.getChannelPointer(0), sample_index, wave_block.getNumSamples());
	return win * velocity_amplitude * samp;
}
float calculatePan(const float pan_latch_val){
	return memoryless::clamp(pan_latch_val, 0.f, 1.f) * std::numbers::pi * 0.5f;
}
void writeAudioToOuts(const float sample, const double fractionalIndex, const float pan_latch_val,
    GrainwisePostProcessing &postProcessing, Grain::outs &outs)
{
	const std::array<float, 2> lr = postProcessing.process(gen::pol2car(sample, pan_latch_val), fractionalIndex);
	outs.audio_L = lr[0];
	outs.audio_R = lr[1];
}
void processBusyness(const float window, gen::history<float> &busyHistory, Grain::outs &outs){
	const float  busy_tmp = window > 0.f;
	outs.busy = busy_tmp;
	busyHistory(busy_tmp);
}
}	// end anonymous namespace

void Grain::setReadBounds(const ReadBounds newReadBounds){
	_upcoming_normalized_read_bounds = newReadBounds;

    // based on settings (?) we can query the corresponding loudness of the source to have a grainwise normalization...
}
void Grain::resetAccum() {
	_accum.reset();
}
void Grain::setAccum(const float newVal) {
	_accum.set(newVal);
}



GrainwisePostProcessing::GrainwisePostProcessing(GranularSynthSharedState *synth_shared_state,
    GranularVoiceSharedState *voice_shared_state)
:   _drive_lgr(voice_shared_state->_gaussian_rng, {1.f, 0.f})
,   _filter_cutoff_lnr(voice_shared_state->_gaussian_rng, {20000.f, 0.f})
,   _filter_q_lgr(voice_shared_state->_gaussian_rng, {0.707f, 0.f})
,   _filter_mode_lgr(voice_shared_state->_gaussian_rng, {0.f, 0.f})
,   _synth_shared_state(synth_shared_state)
{
    jassert(synth_shared_state != nullptr);
    jassert(voice_shared_state != nullptr);
}

std::array<float, 2> GrainwisePostProcessing::process(std::array<float, 2> x, const double fractionalSample)
{
    std::array<float, 2> retval {0.f, 0.f};
    for (size_t i = 0; i < x.size(); ++i){
        retval[i] = processChannel(x[i], fractionalSample, i);
    }
    return retval;
}
void GrainwisePostProcessing::setNormalization(const float norm) {
    _normalization = norm;
}
void GrainwisePostProcessing::setDriveMu(const float muDb) {
    _drive_lgr.setMu(muDb);
}
void GrainwisePostProcessing::setDriveSigma(const float sigmaDb) {
    _drive_lgr.setSigma(sigmaDb);
}
void GrainwisePostProcessing::updateDrive(const bool shouldOpenLatches) {
    const auto d_dB = _drive_lgr(shouldOpenLatches);
    const auto d_rat = std::exp(d_dB * std::numbers::ln10_v<float> / 20.f);
    _drive = d_rat;
    jassert(_drive > 0.0f);
}

void GrainwisePostProcessing::setMakeupGain(const float gain) {
    _makeup_gain = gain;
}

void GrainwisePostProcessing::setFilterCutoffMu(const float hz) {
    _filter_cutoff_lnr.setMu(hz);
}
void GrainwisePostProcessing::setFilterCutoffSigma(const float octaves) {
    // _filter_cutoff_lnr operates in natural-log (neper) space; an octave is a doubling,
    // so a standard deviation of `octaves` octaves is `octaves * ln(2)` nepers.
    static constexpr float nepersPerOctave = std::numbers::ln2_v<float>;
    _filter_cutoff_lnr.setSigma(octaves * nepersPerOctave);
}
void GrainwisePostProcessing::setFilterQMu(const float q) {
    _filter_q_lgr.setMu(q);
}
void GrainwisePostProcessing::setFilterQSigma(const float q) {
    _filter_q_lgr.setSigma(q);
}
void GrainwisePostProcessing::setFilterModeMu(const float modeIndex) {
    _filter_mode_lgr.setMu(modeIndex);
}
void GrainwisePostProcessing::setFilterModeSigma(const float modeIndexSpread) {
    _filter_mode_lgr.setSigma(modeIndexSpread);
}
void GrainwisePostProcessing::updateFilter(const bool shouldOpenLatches) {
    if (!shouldOpenLatches){	// coefficients are cached on the filters between grains; nothing to recompute mid-grain
        return;
    }

    const auto sampleRate = _synth_shared_state->_playback_sample_rate;
    jassert (sampleRate > 0.0);
    const auto nyquist = static_cast<float>(sampleRate * 0.5 - 1.0);

    const float cutoffHz = memoryless::clamp(_filter_cutoff_lnr(shouldOpenLatches), 20.f, nyquist);
    const float q = std::max(_filter_q_lgr(shouldOpenLatches), 0.05f);
    const int modeIdx = juce::jlimit(0, numFilterModes - 1,
        static_cast<int>(std::round(_filter_mode_lgr(shouldOpenLatches))));

    auto const makeCoefficients = [sampleRate, cutoffHz, q](const int mode) {
        switch (static_cast<FilterMode>(mode)) {
            case FilterMode::Bandpass: return juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, cutoffHz, q);
            case FilterMode::Highpass: return juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, cutoffHz, q);
            case FilterMode::Lowpass:
            default:                  return juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffHz, q);
        }
    };

    const auto newCoefficients = makeCoefficients(modeIdx);
    for (auto &filter : _filters){
        filter.coefficients = newCoefficients;
        filter.reset();	// avoid a resonant tail from the previous grain's filter state bleeding into this one
    }
}

float GrainwisePostProcessing::driveStage(float x) const {
    // saturating waveshaper: slope falls off as |v| grows, asymptoting toward +-2.
    auto const shaper = [](const float v) {
        return 2.f * v / (1.f + std::sqrt(1.f + std::abs(v)));
    };

    jassert (_drive > 0.f);
    // _drive scales the signal before shaping, pushing it further out onto the saturating curve
    // (more compression/harmonics). dividing by shaper(_drive) renormalizes so that a full-scale
    // input (|x| == 1) always maps back to ~unity output regardless of how much drive is dialed
    // in -- i.e. drive reshapes the curve the signal traverses without changing the output level
    // at full scale.
    return shaper(x * _drive) / shaper(_drive);
}
float GrainwisePostProcessing::filterStage(float x, const size_t channel) {
    jassert (channel < _filters.size());
    const auto ret = _filters[channel].processSample(x);
    jassert (std::isfinite(ret));
    jassert (ret == ret);
    return ret;
}

float GrainwisePostProcessing::processChannel(float x, double t, const size_t channel) {

    // NEED TO WRAP t
    const auto L = static_cast<double>(_synth_shared_state->_buffer._loudness_profile.size());
    t /= L;
    t = memoryless::mspWrap(t);
    t *= L;

    const auto idx = static_cast<size_t>(t);
    jassert (idx < _synth_shared_state->_buffer._loudness_profile.size());
    const float signal_rms = _synth_shared_state->_buffer._loudness_profile[idx];
    float normalizer = 1.f / std::max(signal_rms, 0.05f);
    static constexpr auto NORMALIZATION_TARGET_AMPLITUDE = 0.33;
    normalizer = _normalization * normalizer * NORMALIZATION_TARGET_AMPLITUDE + (1.f - _normalization);

    x *= normalizer;

    // pipeline stages, applied in order below -- e.g. swap the next two lines to run the filter before drive.
    auto const applyDrive  = [this](const float v) { return driveStage(v); };
    auto const applyFilter = [this, channel](const float v) { return filterStage(v, channel); };

    x = applyDrive(x);
    x = applyFilter(x);

    jassert (_makeup_gain > 0.f);
    return x * _makeup_gain;
}
void Grain::setUnderlyingFundamentalFrequency(const float midi_f0) {
    _underlying_f0 = midi_f0 <= 0 ? 0 :
        util::midiToFrequency(midi_f0,
            _synth_shared_state->_concertPitchHz,
            69.f,
            12);
}

Grain::outs Grain::operator()(const float trig_in){
	assert(_synth_shared_state);
	const dsp::AudioBlock<float> wave_block = _synth_shared_state->_buffer._wave_block;
	const auto playback_sr = _synth_shared_state->_playback_sample_rate;
	const auto file_sr = _synth_shared_state->_buffer._file_sample_rate;
	const auto &settings = _synth_shared_state->_settings;
	
	outs o;
	o.next = _busy_histo.val ? trig_in : 0.f;

    const bool should_reset_accum = _busy_histo.val ? false : static_cast<bool>(trig_in);
    const bool should_open_latches = should_reset_accum || _voice_shared_state->forceGrainTrigger;

    const bool pitchify = _pitchify_latch(_pitchify, should_open_latches);
    const auto f0_compensation_ratio =
        !pitchify ? 1.f :
        _underlying_f0_latch(
            _underlying_f0 > 0 ?
                _synth_shared_state->_concertPitchHz / _underlying_f0  :
                    1.f,
            should_open_latches);

    static constexpr float twelfth = 1.f / 12.f;
	const float randomSemitoneOffset = _frequencyRandomizationMode == FrequencyRandomizationMode::Continuous ? _transpose_lgr(should_open_latches)
	    :   12.f * std::round( _transpose_lgr(should_open_latches) * twelfth );

    if (should_open_latches) {
        _random_pitch_ratio =
            util::midiToFrequency(randomSemitoneOffset-69.f,
                _synth_shared_state->_concertPitchHz,
                69.f,
                _synth_shared_state->_notesPerOctave);
    }
	_waveform_read_rate = calculateTransposeMultiplier(
	    _ratio_for_note_latch(_ratio_based_on_note, should_open_latches),
	    _random_pitch_ratio,
	    f0_compensation_ratio);
	_accum(_waveform_read_rate, should_reset_accum);
	
	const double file_sample_rate_compensate_ratio = calculateSampleReadRate(playback_sr, file_sr);

	_postProcessing.updateDrive(should_open_latches);
	_postProcessing.updateFilter(should_open_latches);

	if (should_open_latches){
		_normalized_read_bounds = _upcoming_normalized_read_bounds;
	    _postProcessing.setNormalization(_grain_normalize_amount);
		_postProcessing.setMakeupGain(_grain_makeup_gain);
	    _duration_pitch_compensation_factor =
	        getDurationPitchCompensationFactor(settings._duration_pitch_compensation, _waveform_read_rate);

	    _duration_in_samps = [this, should_open_latches, playback_sr]()
	    {
	        const auto grain_rate_hz = _grain_rate_latch(_voice_shared_state->grain_rate_hz, should_open_latches);
	        assert(grain_rate_hz > 0.f);
	        const auto grain_base_dur = N_GRAINS / grain_rate_hz;
	        return _density_lnr(should_open_latches) * grain_base_dur * playback_sr;
	    }();
	}

	if (_normalized_read_bounds.end - _normalized_read_bounds.begin == 0.0){	// protection for initialization case
		_window_val = 0.f;
		writeAudioToOuts(0.f, 0.0, 0.f, _postProcessing, o);
		processBusyness(_window_val, _busy_histo, o);
		return o;
	}
	
	const auto buffLength = _synth_shared_state->_buffer._wave_block.getNumSamples();
	ReadBounds denormedReadBounds = _normalized_read_bounds * static_cast<double>(buffLength);
	if (denormedReadBounds.end < denormedReadBounds.begin){
		denormedReadBounds.end += buffLength;	// now this can be longer than the actual number of samples in the buffer. should be taken care of by wrapping in peek().
		assert (denormedReadBounds.end > denormedReadBounds.begin);
	}

	const float latch_skew_result = _skew_lgr(should_open_latches);
// #ifdef DBG
// 	_timed_printer->print("skew result: {}", latch_skew_result);
// #endif

	double norm_pos = [this, should_open_latches](){
		double np = _position_lgr(should_open_latches);
	    const auto &[lfo, shape, amount] = _voice_shared_state->_scanner;
	    const auto phasorVal = (lfo.multi(shape * 4.0f) + 1) * 0.5f;    // this MUST run every sample
		const auto scanner_pos = _scanner_for_position_latch(phasorVal * amount, should_open_latches);
		np = memoryless::mspWrap(np + scanner_pos);
		assert (np >= 0.0);
		assert (np <= 1.0);
		return np;
	}();

    if (should_open_latches) {
        _grainWindow = GrainWindow(
            _duration_in_samps,
            _duration_pitch_compensation_factor,
            latch_skew_result,
            _plateau_lgr(should_open_latches));
    }
	_window_val = _grainWindow.calculate(_accum.val);

	_sample_index = [this, norm_pos, latch_skew_result,
	    file_sample_rate_compensate_ratio, &settings, &denormedReadBounds]()
    {
		// const double normalized_position, const double sr_compensated_duration, const float skew, const float sample_playback_rate, const bool center_envelope_at_env_peak
		const auto center_of_env = calculateCenterOfEnvelope(norm_pos,														// const double normalized_position
															 this->_duration_in_samps,												// const double sr_compensated_duration
															 latch_skew_result,												// const float skew
															 this->_duration_pitch_compensation_factor,							// const float sample_playback_rate
															 settings._center_position_at_env_peak);	// const bool center_envelope_at_env_peak
				
		return calculateSampleIndex(_accum.val,							// const double accum
							 norm_pos,									// const double normalized_position
							 denormedReadBounds.begin, 					// const double sample_left_bound
							 denormedReadBounds.end,					// const double sample_right_bound
							 file_sample_rate_compensate_ratio,			// const double sample_rate_compensate_ratio
							 center_of_env);							// const double center_of_env
	}();
	

	const float sample = [this, &wave_block, should_open_latches](){
		const float vel_amplitude = _amplitude_for_note_latch(_amplitude_based_on_note, should_open_latches)
#ifdef TSN
								* _grain_weight_latch(_grain_weight, should_open_latches);
#endif
		;
	    return calculateSample(wave_block, _sample_index, _window_val, vel_amplitude);
	}();
	_pan = calculatePan(_pan_lgr(should_open_latches));
	
	writeAudioToOuts(sample, _sample_index, _pan, _postProcessing, o);
	
	processBusyness(_window_val, _busy_histo, o);

#ifdef DBG
	if (util::checkNanOrInf(std::array { o.audio_L, o.audio_R })){
		return {};
	}
#endif

	return o;
}
// NOLINTEND(cppcoreguidelines-narrowing-conversions)
}	// namespace nvs::gran
