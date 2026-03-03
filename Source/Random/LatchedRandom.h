/*
  ==============================================================================

    LatchedRandom.h
    Created: 13 Jan 2025 3:55:58pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "Random.h"
#include <concepts>
#include <type_traits>


namespace nvs::rand {

template<std::floating_point float_t>
struct MuSigmaPair {
	float_t mu;
	float_t sigma;
};

template<std::floating_point float_t>
struct LatchedGaussianRandom {
	LatchedGaussianRandom(BoxMuller &rng, MuSigmaPair<float_t> msp)
	:	_rng(rng), _val(msp.mu), _msp(msp){}
	
	float_t operator()(const bool gate){
		if (gate){
			_val = static_cast<float_t>(_rng(static_cast<double>(_msp.mu),
											 static_cast<double>(_msp.sigma)));
		}
		return _val;
	}
	void setMu(const float_t mu){
		_msp.mu = mu;
	}
	void setSigma(const float_t sigma){
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
struct LatchedLogNormalRandom {
    LatchedLogNormalRandom(BoxMuller &rng, MuSigmaPair<float_t> msp)
    :	_rng(rng), _val(msp.mu), _msp(msp){}

    float_t operator()(const bool gate){
        if (gate){
            const auto z = static_cast<float_t>(_rng(0.0, 1.0));    // standard normal
            _val = std::exp(_msp.mu + _msp.sigma * z);
            /* i think technically correct behavior would be:

            _val = std::exp(_msp.mu - (_msp.sigma * _msp.sigma * 0.5f) + _msp.sigma * z);

             but the current way sounds better at least for grain speed
            */
        }
        return _val;
    }
    void setMu(const float_t mu){
        _msp.mu = std::log(mu);
    }
    void setSigma(const float_t sigma){
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
	float_t operator()(const bool gate){
		if (gate){
			_val = static_cast<float_t>(_rng(static_cast<double>(_msp.mu), static_cast<double>(_msp.sigma)));
		}
		return _val;
	}
	void setMu(const float_t mu){
		_msp.mu = mu;
	}
	void setSigma(const float_t sigma){
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
LatchedLogNormalRandom<float_t> createLatchedLogNormalRandom(BoxMuller &rng, MuSigmaPair<float_t> msp) {
    return LatchedLogNormalRandom<float_t>(rng, msp);
}

template <typename float_t>
requires std::floating_point<float_t>
LatchedExponentialRandomWithSigma<float_t> createLatchedExponentialRandom(
    ExponentialRandomNumberGeneratorWithVariance &rng, MuSigmaPair<float_t> msp)
{
    return LatchedExponentialRandomWithSigma<float_t>(rng, msp);
}

}
