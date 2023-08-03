/*
  ==============================================================================

    OnsetAnalysis.h
    Created: 14 Jun 2023 10:28:35am
    Author:  Nicholas Solem

  ==============================================================================
*/

#pragma once
#include "EssentiaSetup.h"
#include "essentia/streaming/algorithms/poolstorage.h"
#include "essentia/scheduler/network.h"
#include "essentia/streaming/algorithms/vectorinput.h"
#include "essentia/streaming/algorithms/vectoroutput.h"
#include "essentia/utils/tnt/tnt2vector.h"
#include "essentia/essentiamath.h"

namespace nvs {
namespace analysis {

//===================================================================================
using namespace essentia;
using namespace essentia::streaming;
using namespace essentia::scheduler;
//===================================================================================
using vecReal = std::vector<Real>;
using vecVecReal = std::vector<vecReal>;
using vectorInput = essentia::streaming::VectorInput<Real> ;
using vectorInputCumulative = essentia::streaming::VectorInput<vecReal> ;
using vectorOutput = essentia::streaming::VectorOutput<Real>;
using vectorOutputCumulative = essentia::streaming::VectorOutput<vecReal> ;
//using matrixInput = essentia::streaming::VectorInput<std::vector<vecReal>> ;
using startAndEndTimesVec = std::pair<vecReal, vecReal> ;

using streamingFactory = essentia::streaming::AlgorithmFactory;
using standardFactory = essentia::standard::AlgorithmFactory;

using array2dReal = TNT::Array2D<Real>;
//===================================================================================
inline void print(char c){
	std::cout << c;
}
template <typename T>
inline void print(T x){
	std::cout << x << ' ';
}
template<typename T>
inline void print(std::vector<T> const &vec){
	for (auto &e : vec){
		print(e);
		print('\n');
	}
}
//===================================================================================
// e.g. vecReal const *sweepVec = new vecReal({ makeSweptSine(100.f, 1000.f, 5000) });
vecReal makeSweptSine(Real const low, Real const high, size_t const len, Real const sampleRate = 44100.f);
//===================================================================================
struct analysisSettings;
struct onsetSettings;
struct bfccSettings;
struct sBicSettings;
struct splitSettings;

//===================================================================================
//vecReal audioFileToVector(std::string filename, streamingFactory const &factory, float sampleRate);
array2dReal onsetAnalysis(vecReal const &waveform, streamingFactory const &factory, analysisSettings const &settings);
vecReal onsetsInSeconds(array2dReal onsetAnalysisMatrix, standardFactory const &factory, onsetSettings const &settings);
vecVecReal featuresForSbic(vecReal const &waveform, AlgorithmFactory const &factory, bfccSettings const &settings);
inline array2dReal vecVecToArray2dReal(vecVecReal const &vv){
	return essentia::transpose(essentia::vecvecToArray2D(vv));
}
vecReal sBic(array2dReal featureMatrix, standardFactory const &factory, sBicSettings const &settings);
vecVecReal splitWaveIntoEvents(vecReal const &wave, vecReal const &onsetsInSeconds, streamingFactory const &factory, splitSettings const &settings);
void writeWav(vecReal const &wave, std::string const &name, streamingFactory const &factory, analysisSettings const &settings);

}	// namespace analysis
}	// namespace nvs
