/*
  ==============================================================================

    OnsetAnalysis.cpp
    Created: 14 Jun 2023 12:16:36pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "OnsetAnalysis.h"

namespace nvs {
namespace analysis {

struct analysisSettings {
	float sampleRate {44100.f};
	int frameSize {1024};
	int hopSize {512};
};
struct onsetSettings{
	explicit onsetSettings(analysisSettings const &a)	:	as(a){}
	analysisSettings const &as;
	
	float silenceThreshold {0.2f};
	float alpha {0.1f};
	unsigned int numFrames_shortOnsetFilter {5};
	
	float weight_hfc {1.f};
	float weight_complex {1.f};
	float weight_complexPhase {1.f};
	float weight_flux {1.f};
	float weight_melFlux {1.f};
	float weight_rms {1.f};

	std::array<float*, 6> weights {
		&weight_hfc,
		&weight_complex,
		&weight_complexPhase,
		&weight_flux,
		&weight_melFlux,
		&weight_rms
	};
};
struct bfccSettings{
	analysisSettings const &as;
	explicit bfccSettings(analysisSettings const &a)	:	as(a){}
	Real highFrequencyBound {11000.f};
	Real lowFrequencyBound {0.f};
//	int inputSize {1025};	get automatically from incoming spectrum
	
	int liftering {0};
	int numBands {40};
	int numCoefficients{13};
	
	enum normalize_e {
		unit_sum = 0,
		unit_max
	};
	enum spectrumType_e {
		magnitude = 0,
		power
	};
	enum weightingType_e {
		warping = 0,
		linear
	};
	enum dctType_e {
		typeII = 2,
		typeIII = 3
	};
	const static inline std::map<normalize_e, std::string>
	normMap {
		{unit_sum, "unit_sum"},
		{unit_max, "unit_max"}
	};
	const static inline std::map<spectrumType_e, std::string>
	spectrumTypeMap {
		{magnitude, "magnitude"},
		{power, "power"}
	};
	const static inline	std::map<weightingType_e, std::string>
	weightingTypeMap {
		{warping, "warping"},
		{linear, "linear"}
	};
	
	std::string normalizeType {normMap.at(normalize_e::unit_sum)};
	std::string spectrumType {spectrumTypeMap.at(spectrumType_e::power)};
	std::string weightingType {weightingTypeMap.at(weightingType_e::warping)};
	
	int dctType {dctType_e::typeII};
};
struct sBicSettings{
	Real complexityPenaltyWeight {1.5f};
	int incrementFirstPass {60};	// min: 1
	int incrementSecondPass {20};
	int minSegmentLengthFrames {10};
	int sizeFirstPass {300};
	int sizeSecondPass {200};
};
struct splitSettings{
	analysisSettings const &as;
	explicit splitSettings(analysisSettings const &a)	:	as(a){}
	size_t fadeInSamps {0};
	size_t fadeOutSamps {0};
};

vecReal makeSweptSine(Real const low, Real const high, size_t const len, Real const sampleRate){
	std::vector<Real> freqSweep(len, 0.f);
	for (auto i = 0; i < len; ++i){
		Real x = (high - low) * (Real(i)/Real(len)) + low;
		freqSweep[i] = std::sin(2.f * 3.14159265f * (x / sampleRate));
	}
	
	return freqSweep;
}
/*
vecReal audioFileToVector(std::string filename, streamingFactory const &factory, float sampleRate){
	Algorithm* audioload    = factory.create("MonoLoader",
										   "filename", filename,
										   "sampleRate", sampleRate,
										   "downmix", "mix");

	
	vecReal vout(0);
	vectorOutput *vecOut = new vectorOutput(&vout);
	
	audioload->output("audio") >> *vecOut;
	
	Network n(audioload);
	n.run();
	assert(vout.size());
	n.clear();
	
	return vout;
}*/

array2dReal onsetAnalysis(std::vector<Real> const &waveform, streamingFactory const &factory, analysisSettings const &settings){
	auto const sr = settings.sampleRate;
	auto const framesize = settings.frameSize;
	auto const hopsize = settings.hopSize;

	vectorInput *inVec = new vectorInput(&waveform);
	
	Algorithm* frameCutter  = factory.create("FrameCutter",
											 "frameSize", framesize,
											 "hopSize", hopsize,
											 "startFromZero", false,
											 "lastFrameToEndOfFile", true,	// irrelevant unless startFromZero is true
											 "validFrameThresholdRatio", 0.0f);	// at 1, only frames with length frameSize are kept
	
	Algorithm* windowingToFFT = factory.create("Windowing",
											 "normalized", true,
											 "size", framesize,
											 "zeroPhase", false,
											 "zeroPadding", 0,
											 "type", "hamming");	// options: hamming, hann, hannnsgcq, triangular, square, blackmanharris62, blackmanharris70, blackmanharris74, blackmanharris92
	
	Algorithm* FFT			= factory.create("FFT",
											 "size", framesize);
	
	Algorithm* carToPol		= factory.create("CartesianToPolar");
	
	Algorithm* onsetDetectionHfc = factory.create("OnsetDetection",
											"method", "hfc",	// options: hfc, complex, complex_phase, flux, melflux, rms
											   "sampleRate", sr);
	Algorithm* onsetDetectionComplex = factory.create("OnsetDetection",
											"method", "complex",	// options: hfc, complex, complex_phase, flux, melflux, rms
											   "sampleRate", sr);
	Algorithm* onsetDetectionComplexPhase = factory.create("OnsetDetection",
											"method", "complex_phase",	// options: hfc, complex, complex_phase, flux, melflux, rms
											   "sampleRate", sr);
	Algorithm* onsetDetectionFlux = factory.create("OnsetDetection",
											"method", "flux",	// options: hfc, complex, complex_phase, flux, melflux, rms
											   "sampleRate", sr);
	Algorithm* onsetDetectionMelFlux = factory.create("OnsetDetection",
											"method", "melflux",	// options: hfc, complex, complex_phase, flux, melflux, rms
											   "sampleRate", sr);
	Algorithm* onsetDetectionRms = factory.create("OnsetDetection",
											"method", "rms",	// options: hfc, complex, complex_phase, flux, melflux, rms
											   "sampleRate", sr);
	vecReal onsetDetVecHFC;
	vectorOutput *onsetDetsHFC = new vectorOutput(&onsetDetVecHFC);
	vecReal onsetDetVecComplex;
	vectorOutput *onsetDetsComplex = new vectorOutput(&onsetDetVecComplex);
	vecReal onsetDetVecComplexPhase;
	vectorOutput *onsetDetsComplexPhase = new vectorOutput(&onsetDetVecComplexPhase);
	vecReal onsetDetVecFlux;
	vectorOutput *onsetDetsFlux = new vectorOutput(&onsetDetVecFlux);
	vecReal onsetDetVecMelFlux;
	vectorOutput *onsetDetsMelFlux = new vectorOutput(&onsetDetVecMelFlux);
	vecReal onsetDetVecRms;
	vectorOutput *onsetDetsRms = new vectorOutput(&onsetDetVecRms);
	
	*inVec >> frameCutter->input("signal");
	frameCutter->output("frame") >> windowingToFFT->input("frame");
	windowingToFFT->output("frame") >> FFT->input("frame");
	FFT->output("fft") >> carToPol->input("complex");
	
	carToPol->output("magnitude") >> onsetDetectionHfc->input("spectrum");
	carToPol->output("phase")	>> onsetDetectionHfc->input("phase");
	
	carToPol->output("magnitude") >> onsetDetectionComplex->input("spectrum");
	carToPol->output("phase")	>> onsetDetectionComplex->input("phase");
	
	carToPol->output("magnitude") >> onsetDetectionComplexPhase->input("spectrum");
	carToPol->output("phase")	>> onsetDetectionComplexPhase->input("phase");
	
	carToPol->output("magnitude") >> onsetDetectionFlux->input("spectrum");
	carToPol->output("phase")	>> onsetDetectionFlux->input("phase");
	
	carToPol->output("magnitude") >> onsetDetectionMelFlux->input("spectrum");
	carToPol->output("phase")	>> onsetDetectionMelFlux->input("phase");
	
	carToPol->output("magnitude") >> onsetDetectionRms->input("spectrum");
	carToPol->output("phase")	>> onsetDetectionRms->input("phase");
	
	onsetDetectionHfc->output("onsetDetection") >> *onsetDetsHFC;
	onsetDetectionComplex->output("onsetDetection") >> *onsetDetsComplex;
	onsetDetectionComplexPhase->output("onsetDetection") >> *onsetDetsComplexPhase;
	onsetDetectionFlux->output("onsetDetection") >> *onsetDetsFlux;
	onsetDetectionMelFlux->output("onsetDetection") >> *onsetDetsMelFlux;
	onsetDetectionRms->output("onsetDetection") >> *onsetDetsRms;

	assert(onsetDetVecHFC.size() == onsetDetVecComplex.size());
	
	Network n(inVec);
	n.run();
	n.clear();
	
	TNT::Array2D<essentia::Real> onsetsMatrix(6, static_cast<int>(onsetDetVecHFC.size()));
	for (int j = 0; j < onsetDetVecHFC.size(); ++j){
		onsetsMatrix[0][j] = onsetDetVecHFC[j];
		onsetsMatrix[1][j] = onsetDetVecComplex[j];
		onsetsMatrix[2][j] = onsetDetVecComplexPhase[j];
		onsetsMatrix[3][j] = onsetDetVecFlux[j];
		onsetsMatrix[4][j] = onsetDetVecMelFlux[j];
		onsetsMatrix[5][j] = onsetDetVecRms[j];
	}
	return onsetsMatrix;
}

vecReal onsetsInSeconds(array2dReal onsetAnalysisMatrix, standardFactory const &factory, onsetSettings const &settings){
	float frameRate = settings.as.sampleRate / static_cast<float>(settings.as.hopSize);

	essentia::standard::Algorithm* onsetDetectionSeconds = factory.create("Onsets",
									  "frameRate", frameRate,
									  "silenceThreshold", settings.silenceThreshold,
									  "alpha", settings.alpha, // proportion of the mean included to reject smaller peaks-filters very short onsets
									  "delay", settings.numFrames_shortOnsetFilter); // number of frames used to compute the threshold-size of short-onset filter

	vecReal weights(6);
	for (int i = 0; i < weights.size(); ++i){
		if (settings.weights[i] != nullptr)
			weights[i] = *settings.weights[i];
	}
	vecReal onsets;
	onsetDetectionSeconds->input("detections").set(onsetAnalysisMatrix);
	onsetDetectionSeconds->input("weights").set(weights);
	onsetDetectionSeconds->output("onsets").set(onsets);
	
	onsetDetectionSeconds->compute();
	assert(onsets.size());
	return onsets;
}

vecVecReal featuresForSbic(vecReal const &waveform, AlgorithmFactory const &factory, bfccSettings const &settings){
	vectorInput *inVec = new vectorInput(&waveform);
	Algorithm* frameCutter	=	factory.create("FrameCutter",
											   "frameSize", settings.as.frameSize,
											   "hopSize", settings.as.hopSize,
											   "lastFrameToEndOfFile", true,
											   "silentFrames", "keep",
											   "startFromZero", true,
											   "validFrameThresholdRatio", 0.f);
	Algorithm* windowing	=	factory.create("Windowing",
											   "normalized", false,
											   "size", settings.as.frameSize,
											   "zeroPadding", settings.as.frameSize,
											   "type", "hann",
											   "zeroPhase", false);
	Algorithm* spectrum		=	factory.create("PowerSpectrum",
											   "size", settings.as.frameSize * 2);
	Algorithm* bfcc			= 	factory.create("BFCC",
											   "sampleRate", settings.as.sampleRate,
											   "dctType", settings.dctType,
											   "highFrequencyBound", settings.highFrequencyBound,
											   "lowFrequencyBound", settings.lowFrequencyBound,
											   "inputSize", settings.as.frameSize + 1,
											   "liftering", settings.liftering,
											   "normalize", settings.normalizeType,
											   "numberBands", settings.numBands,
											   "numberCoefficients", settings.numCoefficients,
											   "weighting", settings.weightingType,
											   "type", "power",
											   "logType", "dbpow"	// use dbpow if working with power
											   );
	Algorithm* barkFrameAccumulator = factory.create("VectorRealAccumulator");
	Algorithm* bfccFrameAccumulator = factory.create("VectorRealAccumulator");
	
	// for some reason, to get this working in as a connection to FrameAccumulator,
	// these must be vector<vector<vector<Real>>>, and the 1st dimension only has size of 1...
	std::vector<std::vector<vecReal>> barkBands, BFCCs;
	VectorOutput<std::vector<vecReal>> *barkAccumOutput = new VectorOutput<std::vector<vecReal>>(&barkBands);
	VectorOutput<std::vector<vecReal>> *bfccAccumOutput = new VectorOutput<std::vector<vecReal>>(&BFCCs);
	
	*inVec								>>	frameCutter->input("signal");
	frameCutter->output("frame")		>>	windowing->input("frame");
	windowing->output("frame")			>>	spectrum->input("signal");
	spectrum->output("powerSpectrum")	>>	bfcc->input("spectrum");
	bfcc->output("bands")				>>	barkFrameAccumulator->input("data");
	bfcc->output("bfcc")				>>	bfccFrameAccumulator->input("data");
	barkFrameAccumulator->output("array") >> *barkAccumOutput;
	bfccFrameAccumulator->output("array") >> *bfccAccumOutput;
	
	Network n(inVec);
	n.run();
	n.clear();
	
	assert(BFCCs.size() == 1);
	assert(BFCCs[0].size());
	assert(BFCCs[0][0].size());

	return BFCCs[0];	// the only dimension that was used
}

vecReal sBic(array2dReal featureMatrix, standardFactory const &factory, sBicSettings const &settings){
	standard::Algorithm* sbic = factory.create("SBic",
											   "cpw", settings.complexityPenaltyWeight,
											   "inc1", settings.incrementFirstPass,
											   "inc2", settings.incrementSecondPass,
											   "minLength", settings.minSegmentLengthFrames,
											   "size1", settings.sizeFirstPass,
											   "size2", settings.sizeSecondPass);
	// "cpw" 1.5, "inc1" 60, "inc2" 20, "minLength" 10, "size1" 300, size2" 200
	vecReal segmentationVec;
	sbic->input("features").set(featureMatrix);
	sbic->output("segmentation").set(segmentationVec);
	sbic->compute();
	return segmentationVec;
}

vecVecReal splitWaveIntoEvents(vecReal const &wave, vecReal const &onsetsInSeconds, streamingFactory const &factory, splitSettings const &settings){
	vecReal endTimes(onsetsInSeconds.size());
	std::copy(onsetsInSeconds.begin() + 1, onsetsInSeconds.end(), endTimes.begin());
	assert(onsetsInSeconds[1] == endTimes[0]);
	
	Real endOfFile = Real(wave.size() - 1) / settings.as.sampleRate;
	endTimes.back() = endOfFile;
	assert(*(onsetsInSeconds.end() - 1) == *(endTimes.end() - 2));

	Algorithm* slicer = factory.create("Slicer",
									   "timeUnits", "seconds",
									   "sampleRate", settings.as.sampleRate,
									   "startTimes", onsetsInSeconds,
									   "endTimes", endTimes);
	
	vectorInput *waveInput = new vectorInput(&wave);
	vecVecReal waveEvents;
	vectorOutputCumulative *waveEventsOutput = new vectorOutputCumulative(&waveEvents);
	
	*waveInput >> slicer->input("audio");
	slicer->output("frame") >> *waveEventsOutput;
	
	Network n(waveInput);
	n.run();
	n.clear();
	
	assert(waveEvents.size());
	
	for (int i = 0; i < waveEvents.size(); ++i){
		size_t currentLength = waveEvents[i].size();
		size_t fadeInSamps = std::min(settings.fadeInSamps, currentLength);
		for (int j = 0; j < fadeInSamps; ++j){
			waveEvents[i][j] = waveEvents[i][j] * ((Real)j / (Real)fadeInSamps);
		}
		size_t fadeOutSamps = std::min(settings.fadeOutSamps, currentLength);
		for (int j = 0; j < fadeOutSamps; ++j){
			size_t currentIdx = (currentLength - 1) - j;
			waveEvents[i][currentIdx] = waveEvents[i][currentIdx] * ((Real)j / (Real)fadeOutSamps);
		}
	}
	
	return waveEvents;
}

void writeWav(vecReal const &wave, std::string const &name, streamingFactory const &factory, analysisSettings const &settings){
	Algorithm* writer = factory.create("MonoWriter",
									   "filename", name,
									   "format", "wav",
									   "sampleRate", settings.sampleRate);
	vectorInput *waveInput = new vectorInput(&wave);
	*waveInput >> writer->input("audio");
	
	Network n(waveInput);
	n.run();
	n.clear();
}

}	// namespace analysis
}	// namespace nvs
