/*
  ==============================================================================

    LatchedRandom.h
    Created: 13 Jan 2025 3:55:58pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "./Random.h"
#include <concepts>
#include <type_traits>


namespace nvs::rand {

template<std::floating_point float_t>
struct MuSigmaPair {
	float_t mu;
	float_t sigma;
};

template <typename T, typename float_t>
concept LatchedRandomWithMuSigma = std::floating_point<float_t> &&
requires(T t, T const ct, bool gate, float_t mu, float_t sigma)
{
	{ t(gate) } -> std::convertible_to<float_t>; // operator()(bool) -> float_t
	
	{ t.setMu(mu) } -> std::same_as<void>;
	{ t.setSigma(sigma) } -> std::same_as<void>;
	{ t.getMu() } -> std::same_as<void>;
	{ ct.getMu() } -> std::same_as<float_t>;
	{ ct.getSigma() } -> std::same_as<float_t>;
};

template<std::floating_point float_t>
struct LatchedGaussianRandom {
	LatchedGaussianRandom(BoxMuller &rng, MuSigmaPair<float_t> msp)
	:	_rng(rng), _val(msp.mu), _msp(msp){}
	
	float_t operator()(bool gate){
		if (gate){
			_val = static_cast<float_t>(_rng(static_cast<double>(_msp.mu),
											 static_cast<double>(_msp.sigma)));
		}
		return _val;
	}
	void setMu(float_t mu){
		_msp.mu = mu;
	}
	void setSigma(float_t sigma){
		_msp.sigma = sigma;
	}
	float_t getMu() const {
		return _msp.mu;
	}
	float_t getSigma() const {
		return _msp.sigma;
	}
	BoxMuller &_rng;
private:
	float_t _val;
	MuSigmaPair<float_t> _msp;
};

template<std::floating_point float_t>
struct LatchedExponentialRandomWithSigma {
	LatchedExponentialRandomWithSigma(ExponentialRandomNumberGeneratorWithVariance &rng, MuSigmaPair<float_t> msp)
	:	_rng(rng), _val(msp.mu), _msp(msp)
	{}
	float_t operator()(bool gate){ return 0.0; /*TODO*/}
	void setMu(float_t mu){
		_msp.mu = mu;
	}
	void setSigma(float_t sigma){
		_msp.sigma = sigma;
	}
	float_t getMu() const {
		return _msp.mu;
	}
	float_t getSigma() const {
		return _msp.sigma;
	}
	ExponentialRandomNumberGeneratorWithVariance &_rng;
private:
	float_t _val;
	MuSigmaPair<float_t> _msp;
};

template <typename float_t>
requires std::floating_point<float_t>
LatchedGaussianRandom<float_t> createLatchedGaussianRandom(BoxMuller &rng, MuSigmaPair<float_t> msp) {
    return LatchedGaussianRandom<float_t>(rng, msp);
}

template <typename float_t>
requires std::floating_point<float_t>
LatchedExponentialRandomWithSigma<float_t> createLatchedExponentialRandom(ExponentialRandomNumberGeneratorWithVariance &rng, MuSigmaPair<float_t> msp) {
    return LatchedExponentialRandomWithSigma<float_t>(rng, msp);
}

class ClassThatNeedsRandomness {
public:
    ClassThatNeedsRandomness(BoxMuller &gaussianRng, ExponentialRandomNumberGeneratorWithVariance &exponentialRng, MuSigmaPair<double> msp)
    : latchedGaussianRandom(createLatchedGaussianRandom(gaussianRng, msp)),
      latchedExponentialRandomWithSigma(createLatchedExponentialRandom(exponentialRng, msp)) {}

    //... functions that use the randomness
private:
	using GaussianRandomType = decltype(createLatchedGaussianRandom(std::declval<BoxMuller&>(), std::declval<MuSigmaPair<double>>()));
	using ExponentialRandomType = decltype(createLatchedExponentialRandom(std::declval<ExponentialRandomNumberGeneratorWithVariance&>(), std::declval<MuSigmaPair<double>>()));
    GaussianRandomType latchedGaussianRandom;
    ExponentialRandomType latchedExponentialRandomWithSigma;
};

}
