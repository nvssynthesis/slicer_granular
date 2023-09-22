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
_grains(_numGrains, genGrain1(wavespan, &_rng, &_numGrains) ),
_grainIndices(_numGrains),
_phasorInternalTrig(sampleRate),
_speedRandomLatch(1.f)
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
	std::shuffle(_grainIndices.begin(), _grainIndices.end(), _rng.getGenerator());
}

void genGranPoly1::setTranspose(float transpositionSemitones){
	float rat = fastSemitonesToRatio(transpositionSemitones);
	for (auto &g : _grains)
		g.setTranspose(rat);
}
void genGranPoly1::setPosition(double positionNormalized){
//	positionNormalized = nvs::memoryless::clamp<double>(positionNormalized, 0.0, 1.0);
	auto pos = positionNormalized * static_cast<double>(_wavespan.size());
	_offsetHisto(pos);
	for (auto &g : _grains)
		g.setOffset(_offsetHisto.val);
}
void genGranPoly1::setSpeed(float newSpeed){
	newSpeed = nvs::memoryless::clamp<float>(newSpeed, 0.f, _sampleRate/2.f);
	_speedHisto(newSpeed);
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
		g.setOffsetRand(randomness);
}
void genGranPoly1::setDurationRandomness(double randomness){
	for (auto &g : _grains)
		g.setDurationRand(randomness);
}
void genGranPoly1::setSpeedRandomness(float randomness){
	// because this value is added as multiplier of speed. we do not want speed to become 0, or it will freeze all grains.
	randomness = nvs::memoryless::clamp_low(randomness, -0.99f);
	_speedRandomness = randomness;
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
	
	float freq_tmp = _speedHisto.val;
	float randFreq = _speedRandomLatch( _speedRandomness * _rng(), _triggerHisto.val );
	randFreq *= freq_tmp;
	freq_tmp += randFreq;
	assert(freq_tmp > 0.f);
	
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

genGrain1::genGrain1(std::span<float> const &waveSpan, RandomNumberGenerator<float> *const ng, size_t *const numGrains, int newId)
:	_waveSpan(waveSpan)
,	grainId(newId)
,	_numGrains_ptr(numGrains)
,	_rng_ptr(ng)
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
	_transpRat = ratio;
}
void genGrain1::setDuration(double duration){
	_duration = duration;
}
void genGrain1::setOffset(double offset){
	_offset = offset;
}
void genGrain1::setSkew(float skew){
	_skew = skew;
}
void genGrain1::setPlateau(float plateau){
	_plateau = plateau;
}
void genGrain1::setPan(float pan){
	_pan = pan;
}
void genGrain1::setTransposeRand(float transposeRand){
	_transposeRand = transposeRand;
}
void genGrain1::setDurationRand(double durationRand){
	_durationRand = durationRand;
}
void genGrain1::setOffsetRand(double offsetRand){
	_offsetRand = offsetRand;
}
void genGrain1::setSkewRand(float skewRand){
	_skewRand = skewRand;
}
void genGrain1::setPlateauRand(float plateauRand){
	_platRand = plateauRand;
}
void genGrain1::setPanRand(float panRand){
	_panRand = panRand;
}

genGrain1::outs genGrain1::operator()(float trig_in){
	outs o;
	
	using namespace nvs::gen;
	// return 1 if histo = TRUE, 2 if histo = FALSE
	const float switch2 = switcher(_histo.val, 1.f, 2.f);
	
	const std::array<float, 2> gater = gateSelect<float, 2>(switch2, trig_in);
	
	// offset, duration ('slope' in max patch), pan, skew
	float transposeEffectiveRandomValue {1.f};
	double offsetEffectiveRandomValue {0.0};
	double durationEffectiveRandomValue {0.0};
	float panEffectiveRandomValue {0.5f};
	float skewEffectiveRandomValue {0.f};
	float plateauEffectiveRandomValue {0.f};
	// get all randoms
	if ((gater[1]) && (_rng_ptr != nullptr)){
		transposeEffectiveRandomValue = (*_rng_ptr)() * _transposeRand;
		transposeEffectiveRandomValue = fastSemitonesToRatio(transposeEffectiveRandomValue);
		assert(transposeEffectiveRandomValue > 0);
		assert(transposeEffectiveRandomValue == transposeEffectiveRandomValue);
		offsetEffectiveRandomValue = static_cast<double>((*_rng_ptr)()) * _offsetRand;
		durationEffectiveRandomValue = static_cast<double>((*_rng_ptr)()) * _durationRand;
		panEffectiveRandomValue = nvs::memoryless::unibi<float>((*_rng_ptr)()) * _panRand;
		panEffectiveRandomValue *= 0.5f; 	// range between [-0.5 .. 0.5]
		assert(panEffectiveRandomValue >= -0.5f);
		assert(panEffectiveRandomValue <= 0.5f);
		skewEffectiveRandomValue = (*_rng_ptr)() * _skewRand;
		skewEffectiveRandomValue *= 0.5f;
		plateauEffectiveRandomValue = (*_rng_ptr)() * 5.f * _platRand;
	}
	
	o.next = gater[0];
	const float ratioBasedOnNote = _ratioForNoteLatch(_ratioBasedOnNote, gater[1]);
	const float transposeEffectiveTotalMultiplier = _transpRat * ratioBasedOnNote * transposeEffectiveRandomValue;
	const float latch_transpose_result = _transposeLatch(transposeEffectiveTotalMultiplier, gater[1]);
	_accum(latch_transpose_result, static_cast<bool>(gater[1]));
	const double accumVal = _accum.val;
	
	const double offset_tmp = _offset + offsetEffectiveRandomValue;
	const double latch_offset_result = _offsetLatch(offset_tmp, gater[1]);

	const double duration_tmp = _duration + durationEffectiveRandomValue*_duration;
	const double latch_duration_result = _durationLatch(duration_tmp, gater[1]);
	
	const double sampleIndex = accumVal + latch_offset_result - (0.5 * latch_duration_result);
	float sample = nvs::gen::peek<float, interpolationModes_e::hermite, boundsModes_e::wrap>(
																 _waveSpan.data(), sampleIndex, _waveSpan.size());
	
	assert(latch_transpose_result > 0.f);
	const float windowIdx_unbounded = (latch_duration_result * accumVal / latch_transpose_result);
	const float windowIdx = nvs::memoryless::clamp<float>(windowIdx_unbounded, 0.f, 1.f);
	
	const float skew_tmp = _skew + skewEffectiveRandomValue;
	const float latch_skew_result = _skewLatch(skew_tmp, gater[1]);
	float win = nvs::gen::triangle<float, false>(windowIdx, latch_skew_result);	// calling fmod because does not assume bounded input
	
	const float plateau_tmp = _plateau + plateauEffectiveRandomValue;
	const float plateau_latch_result = _plateauLatch(plateau_tmp, gater[1]);
	win *= plateau_latch_result;
	win = nvs::memoryless::clamp_high(win, 1.f);
	
	win = nvs::gen::parzen(win);
	win /= plateau_latch_result;
	sample *= win;
	
	const float velAmplitude = _amplitudeForNoteLatch(_amplitudeBasedOnNote, gater[1]);
	sample *= velAmplitude;
	
	float pan_tmp = _pan + panEffectiveRandomValue;
	pan_tmp = pan_tmp;
	float latch_pan_result = _panLatch(pan_tmp, gater[1]);
	
	latch_pan_result = nvs::memoryless::clamp<float>(latch_pan_result, 0.f, 1.f);
	latch_pan_result *= 1.570796326794897f;
	const std::array<float, 2> lr = nvs::gen::pol2car<float>(sample, latch_pan_result);
	o.audio = 	lr[0];
	o.audio_R = lr[1];
	
	const float busy_tmp = (win > 0.f);
	o.busy = busy_tmp;
	
	// original first takes care of denorm...
	_histo(busy_tmp);
	return o;
}

}	// namespace gran
}	// namespace nvs
