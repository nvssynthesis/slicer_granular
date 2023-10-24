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
#include "dsp_util.h"

namespace nvs {
namespace gran {

template <typename float_t>
float semitonesToRatio(float_t semitones){
	constexpr float_t semitoneRatio = static_cast<float_t>(1.059463094359295);
	float_t transpositionRatio = std::pow(semitoneRatio, semitones);
	return transpositionRatio;
}
inline float fastSemitonesToRatio(float semitones){
	return semitonesRatioTable(semitones);
}
genGranPoly1::genGranPoly1(float const &sampleRate, std::span<float> const &wavespan, size_t nGrains)
:
_sampleRate(sampleRate),
_wavespan(wavespan),
_numGrains(nGrains),
_normalizer(1.f / std::sqrt(static_cast<float>(std::clamp(nGrains, 1UL, 10000UL)))),
_grains(_numGrains, genGrain1(wavespan, &_gaussian_rng, &_numGrains) ),
_grainIndices(_numGrains),
_phasorInternalTrig(sampleRate)
{
	for (int i = 0; i < _numGrains; ++i){
		_grains[i].setId(i);
	}
	std::iota(_grainIndices.begin(), _grainIndices.end(), 0);
}

void genGranPoly1::noteOn(noteNumber_t note, velocity_t velocity){
	// reassign to noteHolder
	auto p = std::make_pair(note, velocity);

	noteHolder.insert(p);
	updateNotes();
}
void genGranPoly1::noteOff(noteNumber_t note){
	// remove from noteHolder
//	auto p = std::make_pair(note, 0);
	noteHolder[note] = 0;
	updateNotes();
	noteHolder.erase(note);
	updateNotes();
}
void genGranPoly1::updateNotes(){
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
void genGranPoly1::shuffleIndices(){
	std::shuffle(_grainIndices.begin(), _grainIndices.end(), _gaussian_rng.getGenerator());
}

void genGranPoly1::setTranspose(float transpositionSemitones){
	float rat = fastSemitonesToRatio(transpositionSemitones);
	for (auto &g : _grains)
		g.setTranspose(rat);
}
void genGranPoly1::setPosition(double positionNormalized){
//	positionNormalized = nvs::memoryless::clamp<double>(positionNormalized, 0.0, 1.0);
	auto pos = positionNormalized * static_cast<double>(_wavespan.size());
	_positionHisto(pos);
	for (auto &g : _grains)
		g.setPosition(_positionHisto.val);
}
void genGranPoly1::setSpeed(float newSpeed){
	newSpeed = nvs::memoryless::clamp<float>(newSpeed, 0.f, _sampleRate/2.f);
	speed_lgr.setMu(newSpeed);
}

void genGranPoly1::setDuration(double dur_ms){
	auto freq_samps = durationMsToFreqSamps(dur_ms);
	_durationHisto(freq_samps);
	for (auto &g : _grains)
		g.setDuration(_durationHisto.val);
}
void genGranPoly1::setSkew(float skew){
	skew = nvs::memoryless::clamp<float>(skew, 0.001, 0.999);
	for (auto &g : _grains)
		g.setSkew(skew);
}
void genGranPoly1::setPlateau(float plateau){
	for (auto &g : _grains)
		g.setPlateau(plateau);
}
void genGranPoly1::setPan(float pan){
	for (auto &g : _grains)
		g.setPan(pan);
}

void genGranPoly1::setTransposeRandomness(float randomness){
	for (auto &g : _grains)
		g.setTransposeRand(randomness);
}
void genGranPoly1::setPositionRandomness(double randomness){
	randomness = randomness * static_cast<double>(_wavespan.size());
	for (auto &g : _grains)
		g.setPositionRand(randomness);
}
void genGranPoly1::setDurationRandomness(double randomness){
	for (auto &g : _grains)
		g.setDurationRand(randomness);
}
void genGranPoly1::setSpeedRandomness(float randomness){
	speed_lgr.setSigma(randomness);
}
void genGranPoly1::setSkewRandomness(float randomness){
	for (auto &g : _grains)
		g.setSkewRand(randomness);
}
void genGranPoly1::setPlateauRandomness(float randomness){
	for (auto &g : _grains)
		g.setPlateauRand(randomness);
}
void genGranPoly1::setPanRandomness(float randomness){
	for (auto &g : _grains)
		g.setPanRand(randomness);
}
std::array<float, 2> genGranPoly1::operator()(float triggerIn){
	std::array<float, 2> output;
	
//	float freq_tmp = _speedHisto.val;
//	float randFreq = _speedRandomLatch(_gaussian_rng(, _speedRandomness), _triggerHisto.val );
//	randFreq *= freq_tmp;
//	freq_tmp += randFreq;
	
	
	// update phasor's frequency only if _triggerHisto.val is true
	float freq_tmp = speed_lgr(static_cast<bool>(_triggerHisto.val));
	freq_tmp = nvs::memoryless::clamp_low(freq_tmp, 0.1f);	// once every 10 seconds
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

genGrain1::genGrain1(std::span<float> const &waveSpan, nvs::rand::BoxMuller *const gaussian_rng, size_t *const numGrains, int newId)
:	_waveSpan(waveSpan)
,	_gaussian_rng_ptr(gaussian_rng)
,	grainId(newId)
,	_numGrains_ptr(numGrains)
,	transpose_lgr(*_gaussian_rng_ptr, {0.f, 0.f})
,	position_lgr(*_gaussian_rng_ptr, {0.0, 0.0})
,	duration_lgr(*_gaussian_rng_ptr, {500.f, 0.f})
,	skew_lgr(*_gaussian_rng_ptr, {0.5f, 0.f})
,	plateau_lgr(*_gaussian_rng_ptr, {1.f, 0.f})
,	pan_lgr(*_gaussian_rng_ptr, {0.5f, 0.f})
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
	duration_lgr.setMu(duration);
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
	
	using namespace nvs::gen;
	// return 1 if histo = TRUE, 2 if histo = FALSE
	const float switch2 = switcher(_busyHisto.val, 1.f, 2.f);
	
	const std::array<float, 2> gater = gateSelect<float, 2>(switch2, trig_in);
	
	o.next = gater[0];
	const float ratioBasedOnNote = _ratioForNoteLatch(_ratioBasedOnNote, gater[1]);
	const float transpose_tmp = transpose_lgr(gater[1]);
	const float ratioBasedOnTranspose = fastSemitonesToRatio(transpose_tmp);
	
	const float latch_result_transposeEffectiveTotalMultiplier = ratioBasedOnNote * ratioBasedOnTranspose;
	assert(latch_result_transposeEffectiveTotalMultiplier > 0.f);

	_accum(latch_result_transposeEffectiveTotalMultiplier, static_cast<bool>(gater[1]));
	const double accumVal = _accum.val;
	
	const double latch_position_result = position_lgr(gater[1]);
	assert(latch_position_result >= 0.f);
	const double latch_duration_result = duration_lgr(gater[1]);
	
	const double sampleIndex = accumVal + latch_position_result - (0.5 * latch_duration_result);
	float sample = nvs::gen::peek<float, interpolationModes_e::hermite, boundsModes_e::wrap>(
																 _waveSpan.data(), sampleIndex, _waveSpan.size());
	
	assert(latch_result_transposeEffectiveTotalMultiplier > 0.f);
	double windowIdx = (latch_duration_result * accumVal / latch_result_transposeEffectiveTotalMultiplier);
	if (windowIdx > 0.0){
		if (windowIdx < 1.0){
			int i = 0;
		}
	}
	windowIdx = nvs::memoryless::clamp<double>(windowIdx, 0.0, 1.0);
	

	
	const float latch_skew_result = skew_lgr(gater[1]);
	float win = nvs::gen::triangle<float, false>(static_cast<float>(windowIdx), latch_skew_result);
	
	const float plateau_latch_result = plateau_lgr(gater[1]);
	win *= plateau_latch_result;
	win = nvs::memoryless::clamp_high(win, 1.f);
	win = nvs::gen::parzen(win);
	win /= plateau_latch_result;
	if (win > 0.f){
		int i = 0;
	}
	sample *= win;
	
	const float velAmplitude = _amplitudeForNoteLatch(_amplitudeBasedOnNote, gater[1]);
	sample *= velAmplitude;
	
//	float pan_tmp = _pan + panEffectiveRandomValue;
//	pan_tmp = pan_tmp;
	float latch_pan_result = pan_lgr(gater[1]);
	
	latch_pan_result = nvs::memoryless::clamp<float>(latch_pan_result, 0.f, 1.f);
	latch_pan_result *= 1.570796326794897f;
	const std::array<float, 2> lr = nvs::gen::pol2car<float>(sample, latch_pan_result);
	o.audio = 	win;//lr[0];
	o.audio_R = lr[1];
	
	const float busy_tmp = (win > 0.f);
	o.busy = busy_tmp;
	
	// original first takes care of denorm...
	_busyHisto(busy_tmp);
	return o;
}

}	// namespace gran
}	// namespace nvs
