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
	explicit RandomNumberGenerator(unsigned long seed = 1234567890UL);
	double operator()();
	XoshiroCpp::Xoshiro256Plus &getGenerator();
private:
	XoshiroCpp::Xoshiro256Plus xosh;
};

class BoxMuller {
public:
	explicit BoxMuller(unsigned long seed = 1234567890UL);
	double operator()(double mu, double sigma);
	XoshiroCpp::Xoshiro256Plus &getGenerator();

	double nowaste_pol(double mu, double sigma); // faster (~20%)
	double nowaste_st(double mu, double sigma); // slower
	void setNext(double d); // may be useful e.g. for setting to mu
private:
	RandomNumberGenerator rng;
	unsigned int count {0};
	double next{};
	
	std::pair<double, double> standard(double mu, double sigma);
	std::pair<double, double> polar(double mu, double sigma);
};


class ExponentialRandomNumberGenerator {
public:
    explicit ExponentialRandomNumberGenerator(unsigned long seed = 1234567890UL);
    double operator()(double lambda);
    XoshiroCpp::Xoshiro256Plus &getGenerator();

private:
    RandomNumberGenerator rng;  // Underlying uniform RNG
};

class ExponentialRandomNumberGeneratorWithVariance {
public:
    explicit ExponentialRandomNumberGeneratorWithVariance(unsigned long seed = 1234567890UL);
    double operator()(double mu, double variance);
    XoshiroCpp::Xoshiro256Plus &getGenerator();

private:
    ExponentialRandomNumberGenerator rng;
};


}	// namespace nvs::rand
