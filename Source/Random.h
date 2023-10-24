/*
  ==============================================================================

    Random.h
    Created: 20 Oct 2023 2:25:42pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "XoshiroCpp.hpp"
namespace nvs {
namespace rand {

struct RandomNumberGenerator {
public:
	RandomNumberGenerator()	:	xosh(1234567890UL){}
	double operator()(){
		std::uint64_t randomBits = xosh();
		double randomDouble = XoshiroCpp::DoubleFromBits(randomBits);
		return randomDouble;
	}
	XoshiroCpp::Xoshiro256Plus &getGenerator() {
		return xosh;
	}
private:
	XoshiroCpp::Xoshiro256Plus xosh;
};
struct BoxMuller {
	double operator()(double mu, double sigma){
		return polar(mu, sigma).first;
//		return nowaste_pol(mu, sigma);
	}
	XoshiroCpp::Xoshiro256Plus &getGenerator(){
		return rng.getGenerator();
	}
	// faster (~20%)
	double nowaste_pol(double mu, double sigma){
		if (count == 0){
			++count;
			std::pair <double, double> vals = polar(mu, sigma);
			next = vals.first;
			return vals.second;
		}
		else {
			count = 0;
			return next;
		}
	}
	// slower
	double nowaste_st(double mu, double sigma){
		if (count == 0){
			++count;
			std::pair <double, double> vals = standard(mu, sigma);
			next = vals.first;
			return vals.second;
		}
		else {
			count = 0;
			return next;
		}
	}
	// may be useful e.g. for setting to mu
	void setNext(double d){
		next = d;
	}
private:
	RandomNumberGenerator rng;
	unsigned int count {0};
	double next;
	
	std::pair<double, double> standard(double mu, double sigma){
		constexpr double eps = std::numeric_limits<double>::epsilon();
		
		double u1, u2;
		do {
			u1 = rng();
		} while (u1 <= eps); // don't want to take log of less than eps
		
		u2 = rng();
		
		auto const mag = std::sqrt(-2.0 * std::log(u1)) * sigma;
		
		auto const z0 = mag * std::cos(2.0 * M_PI * u2) + mu;
		auto const z1 = mag * std::sin(2.0 * M_PI * u2) + mu;
		
		return std::make_pair(z0, z1);
	}
	std::pair<double, double> polar(double mu, double sigma){
		double u1, u2, s;
		do {
			u1 = rng() * 2.0 - 1.0;
			u2 = rng() * 2.0 - 1.0;
			s = u1*u1 + u2*u2;
		} while ((s == 0.0) || (s >= 1.0));
		
		auto const mag = std::sqrt((-2.0 * std::log(s)) / s) * sigma;
		auto const z0 = u1 * mag + mu;
		auto const z1 = u2 * mag + mu;
		
		return std::make_pair(z0, z1);
	}
};

}	// namespace rand
}	// namespace nvs
