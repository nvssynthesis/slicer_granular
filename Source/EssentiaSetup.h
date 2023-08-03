/*
  ==============================================================================

    EssentiaSetup.h
    Created: 15 Jun 2023 12:24:37pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "essentia/algorithmfactory.h"

namespace nvs {
namespace ess {

struct EssentiaInitializer {
	EssentiaInitializer(){
		essentia::init();
	}
	~EssentiaInitializer(){
		essentia::shutdown();
	}
};

// instantiates Essentia
struct EssentiaHolder {
	EssentiaInitializer &initializer;
	essentia::streaming::AlgorithmFactory& factory;
	EssentiaHolder(EssentiaInitializer &init) :
		initializer(init),
		factory(essentia::streaming::AlgorithmFactory::instance()){}
	~EssentiaHolder() = default;
};

}
}	// namespace nvs
