/*
  ==============================================================================

    GranularSynthesis.h
    Created: 14 Jun 2023 10:28:50am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "OnsetAnalysis.h"
#include "nvs_libraries/include/nvs_gen.h"
#include "/Users/nicholassolem/development/Xoshiro-cpp/XoshiroCpp.hpp"
#include <span>

namespace nvs {
namespace gran {

using namespace nvs::analysis;

#if 0
inline void loadOnsetsSamples(waveHolder &wh, std::vector<size_t> onsetsSamples){
	assert(onsetsSamples.back() <= (wh.waveform.size()));
	wh.onsets = onsetsSamples;
}
inline void loadOnsetsSeconds(waveHolder &wh, vecReal onsetsSeconds, float sampleRate){
	size_t N = onsetsSeconds.size();
	std::vector<size_t> onsetsSamples(N);
	for (auto i = 0; i < N; ++i){
		onsetsSamples[i] = onsetsSeconds[i] * sampleRate;
		size_t prev = std::max(0, i - 1);
		assert(onsetsSamples[i] >= onsetsSamples[prev]);
	}
	loadOnsetsSamples(wh, onsetsSamples);
}
#endif

template<typename T, typename string_t = char>
struct occasionalPrinter {
	size_t count { 10 };
	size_t current {0};
	occasionalPrinter(size_t c){
		count = c;
	}
	void operator()(T x, string_t s = '\n'){
		if (current == 0)
			std::cout << x << s;
		++current;
		if (current >= count){
			current = 0;
		}
	}
};

template<typename T>
struct numberGenerator {
private:
	T state = 0.25f;
	XoshiroCpp::Xoshiro128Plus xosh;
public:
	numberGenerator()	:	xosh(1234567890UL){}
	T operator()(){
		// next random float
		state += 0.91f;
		if ((state > 0.5) && (state > 0.6)){
			state -= 0.123f;
		}
		state = fmod(state, 1.0);
		
		std::uint32_t randomBits = xosh();
		float randomFloat = XoshiroCpp::FloatFromBits(randomBits);
		return (state * 0.f) + (randomFloat * 1.f);
	}
};

struct genGrain1 {
private:
	nvs::gen::history<float> _histo;// history of 'busy' boolean signal, goes to [switch 1 2]
	nvs::gen::latch<float> _slopeLatch;	// latches slope from gate on, goes toward dest windowing
	nvs::gen::latch<float> _offsetLatch;// latches offset from gate on, goes toward dest windowing
	nvs::gen::accum<float> _accum;	// accumulates samplewise and resets from gate on, goes to windowing and sample lookup!
	
	nvs::gen::latch<float> _panLatch;
	
	float _slope = 1.f;
	float _offset = 0.f;
	float _skew = 0.5f;
	float _pan = 0.5f;
	
	float _slopeRand = 0.f;
	float _offsetRand = 0.f;
	float _skewRand = 0.f;
	float _panRand = 0.f;
	
//	vecReal const *_waveVec;
	std::span<float> const *_waveSpan;
	
	int grainId;
	// these pointers are set by containing granular synth
	size_t *const _numGrains_ptr;
	numberGenerator<float> *const _ng_ptr;
	
	occasionalPrinter<float, std::string> printer;
public:
	explicit genGrain1(std::span<float> const *waveSpan, numberGenerator<float> *const ng, size_t *const numGrains = nullptr, int newId = -1)
	:
	_waveSpan(waveSpan),
	grainId(newId),
	_numGrains_ptr(numGrains),
	_ng_ptr(ng),
	printer(800)
	{
	}
	void setId(int newId){
		grainId = newId;
	}
	void setSlope(float slope){
		_slope = slope;
	}
	void setOffset(float offset){
		_offset = offset;
	}
	void setSkew(float skew){
		_skew = skew;
	}
	void setPan(float pan){
		_pan = pan;
	}
	void setSlopeRand(float slopeRand){
		_slopeRand = slopeRand;
	}
	void setOffsetRand(float offsetRand){
		_offsetRand = offsetRand;
	}
	void setSkewRand(float skewRand){
		_skewRand = skewRand;
	}
	void setPanRand(float panRand){
		_panRand = panRand;
	}
	struct outs {
		float next;
		float busy;
		float audio;
		float audio_R;
	};
	
	outs operator()(float trig_in){
		outs o;
		
		using namespace nvs::gen;
		// return 1 if histo = TRUE, 2 if histo = FALSE
		float switch2 = switcher(_histo.val, 1.f, 2.f);
		
		std::array<float, 2> gater = gateSelect<float, 2>(switch2, trig_in);

		o.next = gater[0];
		_accum(1.f, static_cast<bool>(gater[1]));
		
		auto accumVal = _accum.val;
		
		// offset, slope, pan, skew
		std::array<float, 4> tmp_rand = {0.f, 0.f, 0.f, 0.f};
		// this should just get all randoms
		if (_ng_ptr != nullptr){
			tmp_rand[0] = (*_ng_ptr)();	// offset
			tmp_rand[1] = (*_ng_ptr)(); // slope
			tmp_rand[2] = (*_ng_ptr)();	// pan
			tmp_rand[3] = (*_ng_ptr)();	// skew?
		}
		float offsetTmp = _offset + tmp_rand[0] * _offsetRand;
		float latch_offset_result = _offsetLatch(offsetTmp, gater[1]);
		
		float latch_slope_result = _slopeLatch(_slope, gater[1]);
		
		float panTmp = _pan * nvs::memoryless::unibi<float>(tmp_rand[2]);// * _panRand;
		panTmp = nvs::memoryless::biuni<float>(panTmp);
		float latch_pan_result = _panLatch(panTmp, gater[1]);	// overall panning trend
		
		// data race READ
		float sampleIndex = accumVal + latch_offset_result;
		float sample = nvs::gen::peekBuff<float>(_waveSpan->data(), static_cast<size_t>(sampleIndex), _waveSpan->size()); 	//_peek(sampleIndexer)[0];
		
		float windowIdx = (latch_slope_result * accumVal);

		nvs::memoryless::clamp<float>(windowIdx, 0.f, 1.f);
		float win = nvs::gen::triangle(windowIdx, _skew);
		
		win = nvs::gen::parzen(win);
		sample *= win;
		
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
};

struct genGranPoly1 {
private:
	float const &_sampleRate;				// dependent on owning instantiator, subject to change value from above
	std::span<float> const *_wavespan { nullptr };	// dependent on owning instantiator, subject to change address from above
	size_t _numGrains { 16 };
	std::vector<genGrain1> _grains;
	gen::phasor _phasorInternalTrig;
	nvs::gen::history<float> _slopeHisto;
	nvs::gen::history<float> _offsetHisto;
	nvs::gen::history<float> _triggerHisto;
	nvs::gen::ramp2trig<float> _ramp2trig;
	
	nvs::gen::latch<float> _rateRandomLatch;
	float _rateRandomness {0.f};
	
	nvs::gen::change<float> change_test;
	
	numberGenerator<float> _ng;
public:
	explicit genGranPoly1(float const &sampleRate, std::span<float> const *wavespan = nullptr, size_t nGrains = 16)
	:
	_sampleRate(sampleRate),
	_wavespan(wavespan),
	_numGrains(nGrains),
	_grains(_numGrains, genGrain1(wavespan, &_ng, &_numGrains) ),
	_phasorInternalTrig(sampleRate),
	_rateRandomLatch(1.f)
	{
		for (int i = 0; i < _numGrains; ++i){
			_grains[i].setId(i);
		}
	}
	void setRate(float newRate){
		newRate = nvs::memoryless::clamp<float>(newRate, 0.f, _sampleRate/2.f);
		_phasorInternalTrig.setFrequency(newRate);
	}
	void setDuration(float dur_ms){
		dur_ms = nvs::memoryless::clamp_low<float>(dur_ms, 0.f);
		auto dur_samps = (dur_ms / 1000.f) * _sampleRate;

		dur_samps = nvs::memoryless::clamp_low<float>(dur_samps, 1.f);
		float freq_samps = 1.f / dur_samps;
		_slopeHisto(freq_samps);
		for (auto &g : _grains)
			g.setSlope(_slopeHisto.val);
	}
	void setPosition(float positionNormalized){
		positionNormalized = nvs::memoryless::clamp<float>(positionNormalized, 0.f, 1.f);
		auto pos = positionNormalized * static_cast<float>(_wavespan->size());
		_offsetHisto(pos);
		for (auto &g : _grains)
			g.setOffset(_offsetHisto.val);
	}
	void setSkew(float skew){
		skew = nvs::memoryless::clamp<float>(skew, 0.001, 0.999);
		for (auto &g : _grains)
			g.setSkew(skew);
	}
	void setPan(float pan){
		for (auto &g : _grains)
			g.setPan(pan);
	}
	void setPositionRandomness(float randomness){
		randomness = randomness * static_cast<float>(_wavespan->size());
		for (auto &g : _grains)
			g.setOffsetRand(randomness);
	}
	void setSpeedRandomness(float randomness){
		float val = _ng();
		val *= randomness;
		val = std::exp2f(val);
		_rateRandomness = val;
	}
	void setPanRandomness(float randomness){
		for (auto &g : _grains)
			g.setPanRand(randomness);
	}
	std::array<float, 2> operator()(float triggerIn){
		std::array<float, 2> output;
		
		float freqTmp = _rateRandomLatch(_rateRandomness, _triggerHisto.val );
		_phasorInternalTrig.setFrequency(freqTmp);
		++_phasorInternalTrig;
		
		float trig = _ramp2trig(_phasorInternalTrig.getPhase());
		_triggerHisto(trig);

		if ((trig == 0.f) && (triggerIn == 0.f)){
			trig = 0.f;
		} else {
			trig = 1.f;
		}
		
		std::vector<genGrain1::outs> _outs(_numGrains);
		_outs[0] = _grains[0](trig);
		float audioOut = _outs[0].audio;
		float audioOut_R = _outs[0].audio_R;
		float voicesActive = _outs[0].busy;
		
		for (size_t i = 1; i < _numGrains; ++i){
			float currentTrig = _outs[i-1].next;
			_outs[i] = _grains[i](currentTrig);
			audioOut += _outs[i].audio;
			audioOut_R += _outs[i].audio_R;
			voicesActive += _outs[i].busy;
		}
		float samp = audioOut / _numGrains;
		float samp_R = audioOut_R / _numGrains;
		output[0] = samp;
		output[1] = samp_R;
		return output;
	}
};



/*
struct phasor {
private:
	float phase { 0.f };
	float phaseDelta {0.f};	// frequency / samplerate
public:
	inline void setPhase(float ph){
		phase = ph;
	}
	inline float getPhase() const {
		return phase;
	}
	inline void reset(){
		phase = 0.f;
	}
	inline void setPhaseDelta(float pd){
		phaseDelta = pd;
	}
	// may be called every sample, so no check for aliasing or divide by 0
	inline void setFrequency(float frequency, float sampleRate){
		phaseDelta = frequency / sampleRate;
	}
	// prefix increment
	phasor& operator++(){
		phase += phaseDelta;
		while (phase >= 1.f)
			phase -= 1.f;
		while (phase < 0.f)
			phase += 1.f;
		return *this;
	}
	// postfix increment
	phasor operator++(int){
		phasor old = *this;
		phase += phaseDelta;
		while (phase >= 1.f)
			phase -= 1.f;
		while (phase < 0.f)
			phase += 1.f;
		return old;
	}
};
*/
// non-cyclic counter, always counting in samples
struct line {
private:
	unsigned int idxSamps { 0 };
	unsigned int durationSamps { 44100 };
public:
	inline float getPhase() const {
		return static_cast<float>(idxSamps) / durationSamps;
	}
	inline void reset(){
		idxSamps = 0;
	}
	inline void setDuration(unsigned int dur){
		// shift position of idxSamps based on ratio of previous duration and next
		auto phase = getPhase();
		durationSamps = dur;
		idxSamps = phase * static_cast<float>(durationSamps);
	}
	inline void setDuration(float dur, float sampleRate){
		unsigned int ds = static_cast<unsigned int>(dur * sampleRate);
		setDuration(ds);
	}
	inline bool complete(){
		return (idxSamps >= durationSamps);
	}
	// prefix increment
	line& operator++(){
		if (idxSamps < durationSamps)
			++idxSamps;
		return *this;
	}
	// postfix increment
	line operator++(int){
		line old = *this;
		if (idxSamps < durationSamps)
			++idxSamps;
		return old;
	}
};

inline float fractionalIndex(float phase, size_t lower, size_t range){
	assert(phase >= 0.f);
	assert(phase <= 1.f);
	return (phase * static_cast<float>(range)) + static_cast<float>(lower);
}

}	// namespace granular
}	// namespace nvs
