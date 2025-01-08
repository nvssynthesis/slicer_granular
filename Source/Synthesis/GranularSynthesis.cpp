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
_normalizer(1.f / std::sqrt(static_cast<float>(std::clamp(N_GRAINS, 1UL, 10000UL)))),
_grains(N_GRAINS, genGrain1(&_gaussian_rng) ),
_grainIndices(N_GRAINS)
{
	for (int i = 0; i < N_GRAINS; ++i){
		_grains[i].setId(i);
	}
	std::iota(_grainIndices.begin(), _grainIndices.end(), 0);
}
void genGranPoly1::setAudioBlock(juce::dsp::AudioBlock<float> wave_block, double file_sample_rate){
	_waveBlock = wave_block;
	for (auto &g : _grains){
		g.setAudioBlock(_waveBlock, file_sample_rate);
	}
}
void genGranPoly1::setSampleRate(double sample_rate){
	_phasorInternalTrig.setSampleRate(sample_rate);
	for (auto &g : _grains){
		g.setSampleRate(sample_rate);
	}
}

void genGranPoly1::setLogger(std::function<void(const juce::String&)> logger_function){
	logger = logger_function;
	for (auto &g : _grains){
		g.setLogger(logger_function);
	}
}

void genGranPoly1::doNoteOn(noteNumber_t note, velocity_t velocity){
	// reassign to noteHolder
	auto p = std::make_pair(note, velocity);

	noteHolder.insert(p);
	updateNotes();
	_phasorInternalTrig.reset();
}
void genGranPoly1::doNoteOff(noteNumber_t note){
	// remove from noteHolder
//	auto p = std::make_pair(note, 0);
	noteHolder[note] = 0;
	updateNotes();
	noteHolder.erase(note);
	updateNotes();
}
void genGranPoly1::doUpdateNotes(){
	size_t num_notes = noteHolder.size();
	float grainsPerNoteFloor = N_GRAINS / static_cast<float>(num_notes);

	auto begin = _grains.begin();
	float fractional_right_side = 0.f;
	for (auto e : noteHolder){
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
	noteHolder.clear();
}
void genGranPoly1::doShuffleIndices(){
	std::shuffle(_grainIndices.begin(), _grainIndices.end(), _gaussian_rng.getGenerator());
}

void genGranPoly1::doSetTranspose(float transposition_semitones){
	for (auto &g : _grains)
		g.setTranspose(transposition_semitones);
}
void genGranPoly1::doSetSpeed(float new_speed){
	speed_lgr.setMu(nvs::memoryless::clamp<float>(new_speed, 0.01f, 11025.f));
}
void genGranPoly1::doSetPosition(double position_normalized){
	//	position_normalized = nvs::memoryless::clamp<double>(position_normalized, 0.0, 1.0);
	double pos = position_normalized * static_cast<double>(_waveBlock.getNumSamples());
	for (auto &g : _grains)
		g.setPosition(pos);
}
void genGranPoly1::doSetDuration(double dur_ms){
	for (auto &g : _grains)
		g.setDuration(dur_ms);
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
	randomness = randomness * static_cast<double>(_waveBlock.getNumSamples());
	for (auto &g : _grains)
		g.setPositionRand(randomness);
}
void genGranPoly1::doSetDurationRandomness(double randomness){
	for (auto &g : _grains)
		g.setDurationRand(randomness);
}
void genGranPoly1::doSetSpeedRandomness(float randomness){
	randomness *= speed_lgr.getMu();
	speed_lgr.setSigma(randomness);
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
	float const freq_tmp = nvs::memoryless::clamp_low(
							speed_lgr(static_cast<bool>(_triggerHisto.val)),  speed_lgr.getMu() * 0.125f);
	_phasorInternalTrig.setFrequency(freq_tmp);
	++_phasorInternalTrig;
	
	float trig = _ramp2trig(_phasorInternalTrig.getPhase());
	_triggerHisto(trig);
	trig = (trig == 0.f && trigger_in == 0.f) ? 0.f : 1.f;
	
	std::array<genGrain1::outs, N_GRAINS> _outs;

	size_t idx = _grainIndices[0];
	_outs[idx] = _grains[idx](trig);
	float audio_out_L = _outs[idx].audio_L;
	float audio_out_R = _outs[idx].audio_R;
	float voices_active = _outs[idx].busy;
	
	for (size_t i = 1; i < N_GRAINS; ++i){
		idx = _grainIndices.data()[i];
		size_t prevIdx = _grainIndices.data()[i - 1];
		
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

std::vector<double> genGranPoly1::getSampleIndices() const {
	std::vector<double> indices(N_GRAINS);
	for (size_t i = 0; i < N_GRAINS; ++i){
		indices[i] = _grains[i].getCurrentSampleIndex();
	}
	assert(indices.size() == N_GRAINS);
	return indices;
}

//=====================================================================================
genGrain1::genGrain1(nvs::rand::BoxMuller *const gaussian_rng, int newId)
:	_gaussian_rng_ptr(gaussian_rng)
,	grainId(newId)
,	transpose_lgr(*_gaussian_rng_ptr, {0.f, 0.f})
,	position_lgr(*_gaussian_rng_ptr, {0.0, 0.0})
,	duration_lgr(*_gaussian_rng_ptr, {durationMsToGaussianSpace(500.0, 44100.0), 0.f})
,	skew_lgr(*_gaussian_rng_ptr, {0.5f, 0.f})
,	plateau_lgr(*_gaussian_rng_ptr, {1.f, 0.f})
,	pan_lgr(*_gaussian_rng_ptr, {0.5f, 0.23f})
{
	assert (gaussian_rng);
}

void genGrain1::setLogger(std::function<void (const juce::String &)> logger_function){
	logger = std::move(logger_function);
}
void genGrain1::setAudioBlock(juce::dsp::AudioBlock<float> audio_block, double file_sample_rate){
	_waveBlock = audio_block;
	_fileSampleRate = file_sample_rate;
}
void genGrain1::setSampleRate(double sample_rate) {
	_playbackSampleRate = sample_rate;
}

void genGrain1::setRatioBasedOnNote(float ratioForNote){
	_ratioBasedOnNote = ratioForNote;
}
void genGrain1::setAmplitudeBasedOnNote(float velocity){
	assert(velocity <= 2.f);
	assert(velocity >= 0.f);
	_amplitudeBasedOnNote = velocity;
}
void genGrain1::setId(int newId){
	grainId = newId;
}
void genGrain1::setTranspose(float ratio){
	transpose_lgr.setMu(ratio);
}
void genGrain1::setDuration(double duration){
	double const dur_gaus_space = durationMsToGaussianSpace(duration, _playbackSampleRate);
	duration_lgr.setMu(dur_gaus_space);
}
void genGrain1::setPosition(double position){
	position_lgr.setMu(position);
}
void genGrain1::setSkew(float skew){
	skew_lgr.setMu(skew);
}
void genGrain1::setPlateau(float plateau){
	plateau_lgr.setMu(plateau);
}
void genGrain1::setPan(float pan){
	pan_lgr.setMu(pan);
}
void genGrain1::setTransposeRand(float transposeRand){
	transpose_lgr.setSigma(transposeRand);
}
void genGrain1::setDurationRand(double durationRand){
	durationRand *= duration_lgr.getMu();
	duration_lgr.setSigma(durationRand);
}
void genGrain1::setPositionRand(double positionRand){
	position_lgr.setSigma(positionRand);
}
void genGrain1::setSkewRand(float skewRand){
	skew_lgr.setSigma(skewRand);
}
void genGrain1::setPlateauRand(float plateauRand){
	plateau_lgr.setSigma(plateauRand);
}
void genGrain1::setPanRand(float panRand){
	pan_lgr.setSigma(panRand);
}

namespace {	// anonymous namespace for local helper functions
float calculateTransposeMultiplier(float const ratioBasedOnNote, float const ratioBasedOnTranspose){
	return memoryless::clamp(ratioBasedOnNote * ratioBasedOnTranspose, 0.001f, 1000.f);
}
float calculateWindow(double const accum, double const duration, float const transpositionMultiplier, float const skew, float const plateau){
	assert(transpositionMultiplier > 0.f);
	double const windowIdx = nvs::memoryless::clamp((accum * duration) / transpositionMultiplier, 0.0, 1.0);
	float win = nvs::gen::triangle<float, false>(static_cast<float>(windowIdx), skew);
	
	assert (plateau > 0.f);
	win *= plateau;
	win = gen::parzen(tanh(win)) / gen::parzen(tanh(plateau));
	if (plateau < 1.0){
		win = pow(win, 1.0 / plateau);
	}
	return win;
}
double calculateCenterPosition(size_t const wave_size, double const position_latch_val){
	double const wave_size_d = memoryless::clamp_low(static_cast<double>(wave_size), 0.00001);
	double pos_rand_norm = position_latch_val / wave_size_d;
	// tried to use util::get_closest to quantize position, but it chokes up the cpu
	pos_rand_norm *= wave_size_d;
	double const  center_position_in_samps = memoryless::clamp(position_latch_val,
															   0.0, wave_size_d);
	return center_position_in_samps;
}
double calculateDuration(double const duration_latch_val, double const playback_sample_rate){
	double const duration_hz = memoryless::clamp(duration_latch_val, 0.05, 1000.0);	// incorporate  static_cast<double>(wave_size)
	double const  latch_duration_result = durationGaussianToProcessingSpace(duration_hz, playback_sample_rate);
	return latch_duration_result;
}
double calculateSampleReadRate(double const playback_sample_rate, double const file_sample_rate){
	assert(playback_sample_rate > 0.0);
	assert(file_sample_rate > 0.0);
	return file_sample_rate / playback_sample_rate;
}
float calculateSample(juce::dsp::AudioBlock<float> const wave_block, double const sample_index, float const win, float const velocity_amplitude){
	assert(wave_block.getNumChannels() > 0);
	assert(wave_block.getNumSamples() > 0);
	return win * velocity_amplitude * gen::peek<float,
												gen::interpolationModes_e::hermite,
												gen::boundsModes_e::wrap
												>(wave_block.getChannelPointer(0), sample_index, wave_block.getNumSamples());
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
	
	size_t const wave_size = _waveBlock.getNumSamples();
	
	o.next = _busyHisto.val ? trig_in : 0.f;
	bool const should_open_latches = _busyHisto.val ? false : static_cast<bool>(trig_in);
	
	float const transpose_multiplier = calculateTransposeMultiplier(_ratioForNoteLatch(_ratioBasedOnNote, should_open_latches), 							fastSemitonesToRatio(transpose_lgr(should_open_latches)));


	double const center_position_in_samps = calculateCenterPosition(wave_size, position_lgr(should_open_latches));
	double const  latch_duration_result = calculateDuration(duration_lgr(should_open_latches), _playbackSampleRate);
	float const latch_skew_result = memoryless::clamp(skew_lgr(should_open_latches), 0.001f, 0.999f);
	
	double const sample_read_rate = calculateSampleReadRate(_playbackSampleRate, _fileSampleRate);
	
#pragma message("need to test this skew-based sample index offset further")
	_accum(transpose_multiplier, static_cast<bool>(should_open_latches));
	_sampleIndex = (sample_read_rate * _accum.val) + center_position_in_samps - (latch_skew_result * latch_duration_result);
	float const win = calculateWindow(_accum.val, latch_duration_result, transpose_multiplier, latch_skew_result, plateau_lgr(should_open_latches));
	float const vel_amplitude = _amplitudeForNoteLatch(_amplitudeBasedOnNote, should_open_latches);

	float const sample = calculateSample(_waveBlock, _sampleIndex, win, vel_amplitude);
	float const latch_pan_result = calculatePan(pan_lgr(should_open_latches));
	
	writeAudioToOuts(sample, latch_pan_result, o);
	
	processBusyness(win, _busyHisto, o);
	return o;
}

}	// namespace nvs::gran
