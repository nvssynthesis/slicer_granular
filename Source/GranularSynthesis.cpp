/*
  ==============================================================================

    GranularSynthesis.cpp
    Created: 6 Aug 2023 12:44:38pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "GranularSynthesis.h"
namespace nvs {
namespace gran {
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
	auto p = std::make_pair<noteNumber_t, velocity_t> (std::move(note), std::move(velocity));

	noteHolder.insert(p);
	updateNotes();
}
void genGranPoly1::noteOff(noteNumber_t note){
	// remove from noteHolder
	noteHolder.erase(note);
}
void genGranPoly1::updateNotes(){
	size_t numNotes = noteHolder.size();				// 3
	float grainsPerNoteFloor = _numGrains / static_cast<float>(numNotes);	// 50 / 3 = 16.66

	auto begin = _grains.begin();
	float fractionalRightSide = 0.f;
	for (auto e : noteHolder){
		auto left = begin + static_cast<size_t>(fractionalRightSide);
		fractionalRightSide += grainsPerNoteFloor;
		auto right = begin + static_cast<size_t>(fractionalRightSide);
		for (; left != right; ++left){
			float note = e.first;
			float vel = e.second;
			(*left).setRatioBasedOnNote(note);
		}
	}
	std::shuffle(_grainIndices.begin(), _grainIndices.end(), _rng.getGenerator());
}

void genGranPoly1::setTranspose(float transpositionSemitones){
	constexpr float semitoneRatio = 1.059463094359295f;
	auto transpositionRatio = pow(semitoneRatio, transpositionSemitones);
	for (auto &g : _grains)
		g.setTranspose(transpositionRatio);
}
void genGranPoly1::setPosition(float positionNormalized){
	positionNormalized = nvs::memoryless::clamp<float>(positionNormalized, 0.f, 1.f);
	auto pos = positionNormalized * static_cast<float>(_wavespan.size());
	_offsetHisto(pos);
	for (auto &g : _grains)
		g.setOffset(_offsetHisto.val);
}
void genGranPoly1::setSpeed(float newSpeed){
	newSpeed = nvs::memoryless::clamp<float>(newSpeed, 0.f, _sampleRate/2.f);
	_speedHisto(newSpeed);
}
void genGranPoly1::setDuration(float dur_ms){
	dur_ms = nvs::memoryless::clamp_low<float>(dur_ms, 0.f);
	auto dur_samps = (dur_ms / 1000.f) * _sampleRate;
	
	dur_samps = nvs::memoryless::clamp_low<float>(dur_samps, 1.f);
	float freq_samps = 1.f / dur_samps;
	_durationHisto(freq_samps);
	for (auto &g : _grains)
		g.setDuration(_durationHisto.val);
}
void genGranPoly1::setSkew(float skew){
	skew = nvs::memoryless::clamp<float>(skew, 0.001, 0.999);
	for (auto &g : _grains)
		g.setSkew(skew);
}
void genGranPoly1::setPan(float pan){
	for (auto &g : _grains)
		g.setPan(pan);
}

void genGranPoly1::setTransposeRandomness(float randomness){
	for (auto &g : _grains)
		g.setTransposeRand(randomness);
}
void genGranPoly1::setPositionRandomness(float randomness){
	randomness = randomness * static_cast<float>(_wavespan.size());
	for (auto &g : _grains)
		g.setOffsetRand(randomness);
}
void genGranPoly1::setDurationRandomness(float randomness){
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
void genGrain1::setId(int newId){
	grainId = newId;
}
void genGrain1::setTranspose(float ratio){
	_transpRat = ratio;
}
void genGrain1::setDuration(float duration){
	_duration = duration;
}
void genGrain1::setOffset(float offset){
	_offset = offset;
}
void genGrain1::setSkew(float skew){
	_skew = skew;
}
void genGrain1::setPan(float pan){
	_pan = pan;
}
void genGrain1::setTransposeRand(float transposeRand){
	_transposeRand = transposeRand;
}
void genGrain1::setDurationRand(float durationRand){
	_durationRand = durationRand;
}
void genGrain1::setOffsetRand(float offsetRand){
	_offsetRand = offsetRand;
}
void genGrain1::setSkewRand(float skewRand){
	_skewRand = skewRand;
}
void genGrain1::setPanRand(float panRand){
	_panRand = panRand;
}

genGrain1::outs genGrain1::operator()(float trig_in){
	outs o;
	
	using namespace nvs::gen;
	// return 1 if histo = TRUE, 2 if histo = FALSE
	float switch2 = switcher(_histo.val, 1.f, 2.f);
	
	std::array<float, 2> gater = gateSelect<float, 2>(switch2, trig_in);
	
	// offset, duration ('slope' in max patch), pan, skew
	float transposeEffectiveRandomValue {0.f};
	float offsetEffectiveRandomValue {0.f};
	float durationEffectiveRandomValue {0.f};
	float panEffectiveRandomValue {0.5f};
	float skewEffectiveRandomValue {0.f};
	// get all randoms
	if (_rng_ptr != nullptr){
		transposeEffectiveRandomValue = (*_rng_ptr)() * _transposeRand;
		transposeEffectiveRandomValue = pow(2, transposeEffectiveRandomValue);
		assert(transposeEffectiveRandomValue > 0);
		assert(transposeEffectiveRandomValue == transposeEffectiveRandomValue);
		offsetEffectiveRandomValue = (*_rng_ptr)() * _offsetRand;
		durationEffectiveRandomValue = (*_rng_ptr)() * _durationRand;
		panEffectiveRandomValue = nvs::memoryless::unibi<float>((*_rng_ptr)()) * _panRand;
		panEffectiveRandomValue *= 0.5f; 	// range between [-0.5 .. 0.5]
		assert(panEffectiveRandomValue >= -0.5f);
		assert(panEffectiveRandomValue <= 0.5f);
		skewEffectiveRandomValue = (*_rng_ptr)() * _skewRand;
		skewEffectiveRandomValue *= 0.5f;
	}
	
	o.next = gater[0];
	float ratioBasedOnNote = _ratioForNoteLatch(_ratioBasedOnNote, gater[1]);
	float transposeEffectiveTotalMultiplier = _transpRat * ratioBasedOnNote * transposeEffectiveRandomValue;
	float latch_transpose_result = _transposeLatch(transposeEffectiveTotalMultiplier, gater[1]);
	_accum(latch_transpose_result * 1.f, static_cast<bool>(gater[1]));
	auto accumVal = _accum.val;
	
	float offset_tmp = _offset + offsetEffectiveRandomValue;
	float latch_offset_result = _offsetLatch(offset_tmp, gater[1]);

	float duration_tmp = _duration + durationEffectiveRandomValue;
	float latch_duration_result = _durationLatch(duration_tmp, gater[1]);
	
	float sampleIndex = accumVal + latch_offset_result - (0.5 * latch_duration_result);
	float sample = nvs::gen::peekBuff<float>(_waveSpan.data(), static_cast<size_t>(sampleIndex), _waveSpan.size());
	
	assert(latch_transpose_result > 0.f);
	float windowIdx = (latch_duration_result * accumVal / latch_transpose_result);
	windowIdx = nvs::memoryless::clamp<float>(windowIdx, 0.f, 1.f);
	
	float skew_tmp = _skew + skewEffectiveRandomValue;
	float latch_skew_result = _skewLatch(skew_tmp, gater[1]);
	float win = nvs::gen::triangle<float, false>(windowIdx, latch_skew_result);	// calling fmod because does not assume bounded input
	
	win = nvs::gen::parzen(win);
	sample *= win;
	
	float pan_tmp = _pan + panEffectiveRandomValue;
	pan_tmp = pan_tmp;//nvs::memoryless::biuni<float>(pan_tmp);
	float latch_pan_result = _panLatch(pan_tmp, gater[1]);	// overall panning trend
	
	latch_pan_result = nvs::memoryless::clamp<float>(latch_pan_result, 0.f, 1.f);
	latch_pan_result *= 1.570796326794897f;
	std::array<float, 2> lr = nvs::gen::pol2car<float>(sample, latch_pan_result);
	o.audio = 	lr[0];
	o.audio_R = lr[1];
	
	float busy_tmp = (win > 0.f);
	o.busy = busy_tmp;
	
	// original first takes care of denorm...
	_histo(busy_tmp);
	return o;
}

}	// namespace gran
}	// namespace nvs
