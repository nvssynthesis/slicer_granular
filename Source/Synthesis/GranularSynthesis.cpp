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
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include "fmt/core.h"
#endif


namespace nvs::gran {

template <typename float_t>
float semitonesToRatio(float_t semitones){
	constexpr float_t semitoneRatio = static_cast<float_t>(1.059463094359295);
	float_t transpositionRatio = std::pow(semitoneRatio, semitones);
	return transpositionRatio;
}
inline float fastSemitonesToRatio(float semitones){
	return nvs::util::semitonesRatioTable(semitones);
}

genGranPoly1::genGranPoly1(unsigned long seed)
:
_gaussian_rng(seed),
_expo_rng(seed),
_normalizer(1.f / std::sqrt(static_cast<float>(std::clamp(N_GRAINS, 1UL, 10000UL)))),
_grains(N_GRAINS, genGrain1(&_gaussian_rng, &_expo_rng) ),
_grain_indices(N_GRAINS)
{
	for (int i = 0; i < N_GRAINS; ++i){
		_grains[i].setId(i);
	}
	std::iota(_grain_indices.begin(), _grain_indices.end(), 0);
}
void genGranPoly1::setAudioBlock(juce::dsp::AudioBlock<float> wave_block, double file_sample_rate){
	_wave_block = wave_block;
	for (auto &g : _grains){
		g.setAudioBlock(_wave_block, file_sample_rate);
	}
}
void genGranPoly1::setSampleRate(double sample_rate){
	_phasor_internal_trig.setSampleRate(sample_rate);
	for (auto &g : _grains){
		g.setSampleRate(sample_rate);
	}
}

void genGranPoly1::setLogger(std::function<void(const juce::String&)> logger_function){
	_logger = logger_function;
	for (auto &g : _grains){
		g.setLogger(logger_function);
	}
}

void genGranPoly1::doNoteOn(noteNumber_t note, velocity_t velocity){
	// reassign to noteHolder
	auto p = std::make_pair(note, velocity);

	_note_holder.insert(p);
	updateNotes();
	_phasor_internal_trig.reset();
}
void genGranPoly1::doNoteOff(noteNumber_t note){
	// remove from noteHolder
//	auto p = std::make_pair(note, 0);
	_note_holder[note] = 0;
	updateNotes();
	_note_holder.erase(note);
	updateNotes();
}
void genGranPoly1::doUpdateNotes(){
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
void genGranPoly1::doClearNotes(){
	_note_holder.clear();
}
void genGranPoly1::doShuffleIndices(){
	std::shuffle(_grain_indices.begin(), _grain_indices.end(), _gaussian_rng.getGenerator());
}
std::vector<float> genGranPoly1::getBusyStatuses() const {
	std::vector<float> busyStatuses;
	busyStatuses.reserve(N_GRAINS);
	for (auto const &g : _grains){
		busyStatuses.push_back(g.getBusyStatus());
	}
	return busyStatuses;
}
void genGranPoly1::doSetTranspose(float transposition_semitones){
	for (auto &g : _grains)
		g.setTranspose(transposition_semitones);
}
void genGranPoly1::doSetSpeed(float new_speed){
	assert (new_speed > 0.01f);
	assert (new_speed < 11025.f);
	_speed_ler.setMu(new_speed);
}
void genGranPoly1::doSetPosition(double position_normalized){
	//	position_normalized = nvs::memoryless::clamp<double>(position_normalized, 0.0, 1.0);
	for (auto &g : _grains)
		g.setPosition(position_normalized);
}
void genGranPoly1::doSetDuration(double dur_norm){
	for (auto &g : _grains)
		g.setDuration(dur_norm);
}
void genGranPoly1::doSetSkew(float skew){
	skew = nvs::memoryless::clamp<float>(skew, 0.001, 0.999);
	for (auto &g : _grains)
		g.setSkew(skew);
}
void genGranPoly1::doSetPlateau(float plateau){
	for (auto &g : _grains)
		g.setPlateau(plateau);
}
void genGranPoly1::doSetPan(float pan){
	for (auto &g : _grains)
		g.setPan(pan);
}

void genGranPoly1::doSetTransposeRandomness(float randomness){
	randomness *= 24.f;
	for (auto &g : _grains)
		g.setTransposeRand(randomness);
}
void genGranPoly1::doSetPositionRandomness(double randomness){
	for (auto &g : _grains)
		g.setPositionRand(randomness);
}
void genGranPoly1::doSetDurationRandomness(double randomness){
	for (auto &g : _grains)
		g.setDurationRand(randomness);
}
void genGranPoly1::doSetSpeedRandomness(float randomness){
//	randomness *= _speed_ler.getMu();
	_speed_ler.setSigma(randomness);
}
void genGranPoly1::doSetSkewRandomness(float randomness){
	for (auto &g : _grains)
		g.setSkewRand(randomness);
}
void genGranPoly1::doSetPlateauRandomness(float randomness){
	for (auto &g : _grains)
		g.setPlateauRand(randomness);
}
void genGranPoly1::doSetPanRandomness(float randomness){
	for (auto &g : _grains)
		g.setPanRand(randomness);
}

std::array<float, 2> genGranPoly1::doProcess(float trigger_in){
	std::array<float, 2> output;
	
	// update phasor's frequency only if _triggerHisto.val is true
	float const freq_tmp = _speed_ler(static_cast<bool>(_trigger_histo.val)); // used to clamp by percentage of mu. should no longer be necessary.
	_phasor_internal_trig.setFrequency(freq_tmp);
	++_phasor_internal_trig;
	
	float trig = _ramp2trig(_phasor_internal_trig.getPhase());
	_trigger_histo(trig);
	trig = (trig == 0.f && trigger_in == 0.f) ? 0.f : 1.f;
	
	std::array<genGrain1::outs, N_GRAINS> _outs;

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
	return output;
}

std::vector<GrainDescription> genGranPoly1::getGrainDescriptions() const {
	std::vector<GrainDescription> gds(N_GRAINS);
	for (size_t i = 0; i < N_GRAINS; ++i){
		gds[i] = _grains[i].getGrainDescription();
	}
	assert(gds.size() == N_GRAINS);
	return gds;
}

//=====================================================================================
genGrain1::genGrain1(nvs::rand::BoxMuller *const gaussian_rng, ExponentialRandomNumberGenerator *const expo_rng, int newId)
:	_gaussian_rng_ptr(gaussian_rng)
,	_grain_id(newId)
,	_transpose_lgr(*_gaussian_rng_ptr, {0.f, 0.f})
,	_position_lgr(*_gaussian_rng_ptr, {0.0, 0.0})
,	_duration_ler(*expo_rng, {0.5f, 0.f})
,	_skew_lgr(*_gaussian_rng_ptr, {0.5f, 0.f})
,	_plateau_lgr(*_gaussian_rng_ptr, {1.f, 0.f})
,	_pan_lgr(*_gaussian_rng_ptr, {0.5f, 0.23f})
{
	assert (gaussian_rng);
	assert (expo_rng);
}

void genGrain1::setLogger(std::function<void (const juce::String &)> logger_function){
	_logger = std::move(logger_function);
}
void genGrain1::setAudioBlock(juce::dsp::AudioBlock<float> audio_block, double file_sample_rate){
	_wave_block = audio_block;
	_file_sample_rate = file_sample_rate;
}
void genGrain1::setSampleRate(double sample_rate) {
	_playback_sample_rate = sample_rate;
}

void genGrain1::setRatioBasedOnNote(float ratioForNote){
	_ratio_based_on_note = ratioForNote;
}
void genGrain1::setAmplitudeBasedOnNote(float velocity){
	assert(velocity <= 2.f);
	assert(velocity >= 0.f);
	_amplitude_based_on_note = velocity;
}
void genGrain1::setId(int newId){
	_grain_id = newId;
}
void genGrain1::setTranspose(float ratio){
	_transpose_lgr.setMu(ratio);
}
void genGrain1::setDuration(double dur_norm){
	assert (dur_norm <= 1.0);
	assert (dur_norm >= 0.0);
	_duration_ler.setMu(dur_norm);
}
void genGrain1::setPosition(double position){
	_position_lgr.setMu(position);
}
void genGrain1::setSkew(float skew){
	_skew_lgr.setMu(skew);
}
void genGrain1::setPlateau(float plateau){
	_plateau_lgr.setMu(plateau);
}
void genGrain1::setPan(float pan){
	assert ((pan >= 0.f) and (pan <= 1.f));
	// reverse effective control to correspond better with gui representation
	_pan_lgr.setMu(1.f - pan);
}
void genGrain1::setTransposeRand(float transposeRand){
	_transpose_lgr.setSigma(transposeRand);
}
void genGrain1::setDurationRand(double durationRand){
	_duration_ler.setSigma(durationRand);
}
void genGrain1::setPositionRand(double positionRand){
	_position_lgr.setSigma(positionRand);
}
void genGrain1::setSkewRand(float skewRand){
	_skew_lgr.setSigma(skewRand);
}
void genGrain1::setPlateauRand(float plateauRand){
	_plateau_lgr.setSigma(plateauRand);
}
void genGrain1::setPanRand(float panRand){
	_pan_lgr.setSigma(panRand);
}
float genGrain1::getBusyStatus() const{
	return _busy_histo.val;
}

GrainDescription genGrain1::getGrainDescription() const {
	GrainDescription gd;
	gd.grain_id = _grain_id;
	gd.position = nvs::gen::wrap01(_sample_index / _wave_block.getNumSamples());
	gd.sample_playback_rate = _sample_playback_rate;
	gd.window = _window;
	gd.pan = _pan / (std::numbers::pi * 0.5f);
	return gd;
}

namespace {	// anonymous namespace for local helper functions
float calculateTransposeMultiplier(float const ratioBasedOnNote, float const ratioBasedOnTranspose){
	return memoryless::clamp(ratioBasedOnNote * ratioBasedOnTranspose, 0.001f, 1000.f);
}
float calculateWindow(double const accum, double const duration, float const transpositionMultiplier, float const skew, float const plateau){
	assert(transpositionMultiplier > 0.f);
	assert (duration > 0.0);
	double const v = (accum / duration);
	double const windowIdx = nvs::memoryless::clamp(v / transpositionMultiplier, 0.0, 1.0);
	float win = nvs::gen::triangle<float, false>(static_cast<float>(windowIdx), skew);
	
	assert (plateau > 0.f);
	win *= plateau;
	win = gen::parzen(tanh(win)) / gen::parzen(tanh(plateau));
	if (plateau < 1.0){
		win = pow(win, 1.0 / plateau);
	}
	return win;
}
double calculateSampleReadRate(double const playback_sample_rate, double const file_sample_rate){
	assert(playback_sample_rate > 0.0);
	assert(file_sample_rate > 0.0);
	return file_sample_rate / playback_sample_rate;
}
double calculateSampleIndex(double sample_rate_compensate_ratio, double accum, double position, double duration, float skew, float sample_playback_rate){
	double const a = sample_rate_compensate_ratio * accum;
	double const center_of_env = skew * duration * sample_playback_rate;
	double const sample_index = a + position - center_of_env;
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
void writeAudioToOuts(float const sample, float const pan_latch_val, genGrain1::outs &outs){
	std::array<float, 2> const lr = gen::pol2car(sample, pan_latch_val);
	outs.audio_L = lr[0];
	outs.audio_R = lr[1];
}
void processBusyness(float const window, nvs::gen::history<float> &busyHistory, genGrain1::outs &outs){
	float const  busy_tmp = (window > 0.f);
	outs.busy = busy_tmp;
	busyHistory(busy_tmp);
}
}	// end anonymous namespace

genGrain1::outs genGrain1::operator()(float const trig_in){
	outs o;
	assert(_gaussian_rng_ptr != nullptr);
	
	o.next = _busy_histo.val ? trig_in : 0.f;
	bool const should_open_latches = _busy_histo.val ? false : static_cast<bool>(trig_in);
	
	_sample_playback_rate = calculateTransposeMultiplier(_ratio_for_note_latch(_ratio_based_on_note, should_open_latches), 							fastSemitonesToRatio(_transpose_lgr(should_open_latches)));

	size_t const length = static_cast<double>(_wave_block.getNumSamples());
	double const position_in_samps = _position_lgr(should_open_latches) * length;
	double const latch_duration_result = _duration_ler(should_open_latches) * length;
	float const latch_skew_result = memoryless::clamp(_skew_lgr(should_open_latches), 0.001f, 0.999f);
	
	double const sample_rate_compensate_ratio = calculateSampleReadRate(_playback_sample_rate, _file_sample_rate);
	
	_accum(_sample_playback_rate, static_cast<bool>(should_open_latches));
	_sample_index = calculateSampleIndex(sample_rate_compensate_ratio, _accum.val, position_in_samps, latch_duration_result, latch_skew_result, _sample_playback_rate);
	_window = calculateWindow(_accum.val, latch_duration_result, _sample_playback_rate, latch_skew_result, memoryless::clamp_low(_plateau_lgr(should_open_latches), 0.000001f));
	float const vel_amplitude = _amplitude_for_note_latch(_amplitude_based_on_note, should_open_latches);

	float const sample = calculateSample(_wave_block, _sample_index, _window, vel_amplitude);
	_pan = calculatePan(_pan_lgr(should_open_latches));
	
	writeAudioToOuts(sample, _pan, o);
	
	processBusyness(_window, _busy_histo, o);
	return o;
}

}	// namespace nvs::gran
