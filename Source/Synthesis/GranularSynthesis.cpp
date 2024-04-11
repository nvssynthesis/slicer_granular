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

namespace nvs {
namespace gran {

template <typename float_t>
float semitonesToRatio(float_t semitones){
	constexpr float_t semitoneRatio = static_cast<float_t>(1.059463094359295);
	float_t transpositionRatio = std::pow(semitoneRatio, semitones);
	return transpositionRatio;
}
inline float fastSemitonesToRatio(float semitones){
	return nvs::util::semitonesRatioTable(semitones);
}
genGranPoly1::genGranPoly1(double const &sampleRate,
						   std::span<float> const &wavespan, double const &fileSampleRate,
						   size_t nGrains)
:
_sampleRate(sampleRate),
_wavespan(wavespan),
_fileSampleRate(fileSampleRate),
_numGrains(nGrains),
_normalizer(1.f / std::sqrt(static_cast<float>(std::clamp(nGrains, 1UL, 10000UL)))),
_grains(_numGrains, genGrain1(_sampleRate, wavespan, fileSampleRate, &_gaussian_rng, &_numGrains) ),
_grainIndices(_numGrains),
_phasorInternalTrig(_sampleRate)
{
	for (int i = 0; i < _numGrains; ++i){
		_grains[i].setId(i);
	}
	std::iota(_grainIndices.begin(), _grainIndices.end(), 0);
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
	size_t numNotes = noteHolder.size();
	float grainsPerNoteFloor = _numGrains / static_cast<float>(numNotes);

	auto begin = _grains.begin();
	float fractionalRightSide = 0.f;
	for (auto e : noteHolder){
		auto left = begin + static_cast<size_t>(fractionalRightSide);
		fractionalRightSide += grainsPerNoteFloor;
		auto right = begin + static_cast<size_t>(fractionalRightSide);
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
void genGranPoly1::doShuffleIndices(){
	std::shuffle(_grainIndices.begin(), _grainIndices.end(), _gaussian_rng.getGenerator());
}

void genGranPoly1::doSetTranspose(float transpositionSemitones){
	for (auto &g : _grains)
		g.setTranspose(transpositionSemitones);
}
void genGranPoly1::doSetSpeed(float newSpeed){
	newSpeed = nvs::memoryless::clamp<float>(newSpeed, 0.1f, _sampleRate/2.f);
	speed_lgr.setMu(newSpeed);
}
void genGranPoly1::doSetPosition(double positionNormalized){
	//	positionNormalized = nvs::memoryless::clamp<double>(positionNormalized, 0.0, 1.0);
	double pos = positionNormalized * static_cast<double>(_wavespan.size());
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
	randomness = randomness * static_cast<double>(_wavespan.size());
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
std::array<float, 2> genGranPoly1::doProcess(float triggerIn){
	std::array<float, 2> output;
	
	// update phasor's frequency only if _triggerHisto.val is true
	float const freq_tmp = nvs::memoryless::clamp_low(
							speed_lgr(static_cast<bool>(_triggerHisto.val)),
												0.5f);	// once every 2 seconds
	_phasorInternalTrig.setFrequency(freq_tmp);
	++_phasorInternalTrig;
	
	float trig = _ramp2trig(_phasorInternalTrig.getPhase());
	_triggerHisto(trig);
	
	if ((trig == 0.f) && (triggerIn == 0.f)){
		trig = 0.f;
	} else {
		trig = 1.f;
	}
	
	std::vector<genGrain1::outs> _outs(_numGrains);
	size_t idx = _grainIndices[0];
	_outs[idx] = _grains[idx](trig);
	float audioOut = _outs[idx].audio;
	float audioOut_R = _outs[idx].audio_R;
	float voicesActive = _outs[idx].busy;
	
	for (size_t i = 1; i < _numGrains; ++i){
		idx = _grainIndices.data()[i];
		size_t prevIdx = _grainIndices.data()[i - 1];
		
		float currentTrig = _outs[prevIdx].next;
		_outs[idx] = _grains[idx](currentTrig);
		audioOut += _outs[idx].audio;
		audioOut_R += _outs[idx].audio_R;
		voicesActive += _outs[idx].busy;
	}
	float samp = audioOut * _normalizer;
	float samp_R = audioOut_R * _normalizer;
	output[0] = samp;
	output[1] = samp_R;
	return output;
}
//=====================================================================================
genGrain1::genGrain1(double const &sampleRate, std::span<float> const &waveSpan, double const &fileSampleRate,
					 nvs::rand::BoxMuller *const gaussian_rng, size_t *const numGrains,
					 int newId)
:	_sampleRate(sampleRate)
,	_waveSpan(waveSpan)
,	_fileSampleRate(fileSampleRate)
,	_gaussian_rng_ptr(gaussian_rng)
,	grainId(newId)
,	_numGrains_ptr(numGrains)
,	transpose_lgr(*_gaussian_rng_ptr, {0.f, 0.f})
,	position_lgr(*_gaussian_rng_ptr, {0.0, 0.0})
,	duration_lgr(*_gaussian_rng_ptr, {durationMsToGaussianSpace(500.0, 44100.0), 0.f})
,	skew_lgr(*_gaussian_rng_ptr, {0.5f, 0.f})
,	plateau_lgr(*_gaussian_rng_ptr, {1.f, 0.f})
,	pan_lgr(*_gaussian_rng_ptr, {0.5f, 1.f})
{}

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
	double const dur_gaus_space = durationMsToGaussianSpace(duration, _sampleRate);
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

genGrain1::outs genGrain1::operator()(float trig_in){
	outs o;
	if (!_gaussian_rng_ptr){
		o.audio = o.audio_R = 0.f;
		return o;
	}
	
	using namespace nvs;
	size_t const waveSize = _waveSpan.size();
	// return 1 if histo = TRUE, 2 if histo = FALSE
	int switch2;
	if (_busyHisto.val){
		switch2 = 1;
	}
	else {
		switch2 = 2;
	}
	std::array<float, 2> gater;
	
	if (switch2 == 1){
		gater = {trig_in, 0.f};
	}
	else if (switch2 == 2) {
		gater = {0.f, trig_in};
	}
	
	o.next = gater[0];
	float const  ratioBasedOnNote = _ratioForNoteLatch(_ratioBasedOnNote, gater[1]);
	float const  transpose_tmp = transpose_lgr(gater[1]);
	float const  ratioBasedOnTranspose = fastSemitonesToRatio(transpose_tmp);
	
	float const  latch_result_transposeEffectiveTotalMultiplier = memoryless::clamp(
													ratioBasedOnNote * ratioBasedOnTranspose,
															0.001f, 1000.f);

	_accum(latch_result_transposeEffectiveTotalMultiplier, static_cast<bool>(gater[1]));
	double const  accumVal = _accum.val;
	
	double pos_rand_norm = position_lgr(gater[1]) / static_cast<double>(waveSize);
	// tried to use util::get_closest to quantize position, but it chokes up the cpu
	pos_rand_norm *= static_cast<double>(waveSize);
	double const  latch_position_result = memoryless::clamp(position_lgr(gater[1]),
													0.0, static_cast<double>(waveSize));
	
	double constexpr leastDuration_hz = 0.05;
	double constexpr greatestDuration_hz = 1000.f; //incorporate  static_cast<double>(waveSize)
	double const duration_hz = memoryless::clamp(duration_lgr(gater[1]), leastDuration_hz, greatestDuration_hz);
	double const  latch_duration_result = durationGaussianToProcessingSpace(duration_hz, _sampleRate);
	float const latch_skew_result = memoryless::clamp(skew_lgr(gater[1]), 0.001f, 0.999f);
	
	assert(_sampleRate > 0.0);
	assert(_fileSampleRate > 0.0);
	double const sampleReadRate = _fileSampleRate / _sampleRate;
	
#pragma message("need to test this skew-based sample index offset further")
	double const  sampleIndex = (sampleReadRate * accumVal) + latch_position_result - (latch_skew_result * latch_duration_result);
	float sample = gen::peek<float, gen::interpolationModes_e::hermite, gen::boundsModes_e::wrap>(
												_waveSpan.data(), sampleIndex, _waveSpan.size());
	
	assert(latch_result_transposeEffectiveTotalMultiplier > 0.f);
	double const windowIdx = memoryless::clamp(
				   (accumVal * latch_duration_result) / latch_result_transposeEffectiveTotalMultiplier,
													   0.0, 1.0);

	
	float win = nvs::gen::triangle<float, false>(static_cast<float>(windowIdx), latch_skew_result);
	
	float const  plateau_latch_result = plateau_lgr(gater[1]);
	win *= plateau_latch_result;
	win = memoryless::clamp_high(win, 1.f);
	win = gen::parzen(win);
	win /= plateau_latch_result;
	sample *= win;
	
	float const  velAmplitude = _amplitudeForNoteLatch(_amplitudeBasedOnNote, gater[1]);
	sample *= velAmplitude;
	
	float latch_pan_result = pan_lgr(gater[1]);
	
	latch_pan_result = memoryless::clamp<float>(latch_pan_result, 0.f, 1.f);
	latch_pan_result *= 1.570796326794897f;
	std::array<float, 2> const lr = gen::pol2car<float>(sample, latch_pan_result);
	o.audio = 	lr[0];
	o.audio_R = lr[1];
	
	float const  busy_tmp = (win > 0.f);
	o.busy = busy_tmp;
	
	// original first takes care of denorm...
	_busyHisto(busy_tmp);
	return o;
}

}	// namespace gran
}	// namespace nvs
