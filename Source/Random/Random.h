/*
  ==============================================================================

    Random.h
    Created: 20 Oct 2023 2:25:42pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "XoshiroCpp.hpp"
namespace nvs::rand {

class RandomNumberGenerator {
public:
	explicit RandomNumberGenerator(const unsigned long seed = 1234567890UL)	:	xosh(seed){}
	double operator()(){
		const std::uint64_t randomBits = xosh();
		const double randomDouble = XoshiroCpp::DoubleFromBits(randomBits);
		return randomDouble;
	}
	XoshiroCpp::Xoshiro256Plus &getGenerator() {
		return xosh;
	}
private:
	XoshiroCpp::Xoshiro256Plus xosh;
};

class BoxMuller {
public:
	explicit BoxMuller(const unsigned long seed = 1234567890UL)
	:	rng(seed){}
	double operator()(const double mu, const double sigma){
		return polar(mu, sigma).first;
//		return nowaste_pol(mu, sigma);
	}
	XoshiroCpp::Xoshiro256Plus &getGenerator(){
		return rng.getGenerator();
	}
	// faster (~20%)
	double nowaste_pol(const double mu, const double sigma){
		if (count == 0){
			++count;
			const auto [curr, _next] = polar(mu, sigma);
			next = _next;
			return curr;
		}
		count = 0;
		return next;
	}
	// slower
	double nowaste_st(const double mu, const double sigma){
		if (count == 0){
			++count;
			const auto [curr, _next] = standard(mu, sigma);
			next = _next;
			return curr;
		}
		count = 0;
		return next;
	}
	// may be useful e.g. for setting to mu
	void setNext(const double d){
		next = d;
	}
private:
	RandomNumberGenerator rng;
	unsigned int count {0};
	double next{};
	
	std::pair<double, double> standard(const double mu, const double sigma){
		constexpr double eps = std::numeric_limits<double>::epsilon();
		
		double u1;
		do {
			u1 = rng();
		} while (u1 <= eps); // don't want to take log of less than eps
		
		const double u2 = rng();
		
		auto const mag = std::sqrt(-2.0 * std::log(u1)) * sigma;
		
		auto const z0 = mag * std::cos(2.0 * M_PI * u2) + mu;
		auto const z1 = mag * std::sin(2.0 * M_PI * u2) + mu;
		
		return std::make_pair(z0, z1);
	}
	std::pair<double, double> polar(const double mu, const double sigma){
		double u1, u2, s;
		do {
			u1 = rng() * 2.0 - 1.0;
			u2 = rng() * 2.0 - 1.0;
			s = u1*u1 + u2*u2;
		} while (s == 0.0 || s >= 1.0);
		
		auto const mag = std::sqrt(-2.0 * std::log(s) / s) * sigma;
		auto const z0 = u1 * mag + mu;
		auto const z1 = u2 * mag + mu;
		
		return std::make_pair(z0, z1);
	}
};


class ExponentialRandomNumberGenerator {
public:
    explicit ExponentialRandomNumberGenerator(const unsigned long seed = 1234567890UL)
        : rng(seed) {}

    double operator()(const double lambda) {
        double uniformRandom = rng();
        if (uniformRandom == 0.0) {
            uniformRandom = std::numeric_limits<double>::min(); // Smallest positive double
        }

        // Inverse transform sampling to get exponentially distributed random number
        const double expRandom = -std::log(1.0 - uniformRandom) / lambda;

        return expRandom;
    }

    XoshiroCpp::Xoshiro256Plus &getGenerator() {
        return rng.getGenerator();
    }

private:
    RandomNumberGenerator rng;  // Underlying uniform RNG
};

class ExponentialRandomNumberGeneratorWithVariance {
public:
    explicit ExponentialRandomNumberGeneratorWithVariance(const unsigned long seed = 1234567890UL)
    : rng(seed) {}
    double operator()(const double mu, const double variance) {
        auto const lambda = 1.0 / mu;
        auto const expRandom = rng(lambda);
        return variance*expRandom + (1.0 - variance)*mu;
    }
    XoshiroCpp::Xoshiro256Plus &getGenerator() {
        return rng.getGenerator();
    }
private:
    ExponentialRandomNumberGenerator rng;
};


}	// namespace nvs::rand
