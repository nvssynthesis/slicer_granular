//
// Created by Nicholas Solem on 1/16/26.
//

#include "misc_util_juce.h"
#include "StringAxiom.h"

namespace nvs::util
{

/*
 LoggingGuts::LoggingGuts wants to be defined in the corresponding PluginProcessor that uses the logger! This way, the logFile can have an appropriate name.
 */
LoggingGuts::~LoggingGuts()
{
	fileLogger.trimFileSize(logFile , 64 * 1024);
	juce::Logger::setCurrentLogger (nullptr);
}
void LoggingGuts::logIfNaNOrInf(juce::AudioBuffer<float> buffer){
	float rms = 0.f;
	for (auto ch = 0; ch < buffer.getNumChannels(); ++ch){
		rms += buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
	}
	if (rms != rms) {
		fileLogger.writeToLog("processBlock:						rms was NaN");
	}
	if (std::isinf(rms)) {
		fileLogger.writeToLog("processBlock:						rms was Inf");
	}
}

SampleManagementGuts::SampleManagementGuts()
{
	formatManager.registerBasicFormats();
}
SampleManagementGuts::~SampleManagementGuts()
{
	formatManager.clearFormats();
}

bool SampleManagementGuts::loadAudioFile(const juce::File& file)
{
	clear();

	auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
	if (!reader) {
		DBG("could not read file\n");
		return false;
	}
	const auto lengthInSamps = static_cast<int>(reader->lengthInSamples);

	sampleBuffer.setSize(reader->numChannels, lengthInSamps);
	reader->read(sampleBuffer.getArrayOfWritePointers(),
				 reader->numChannels,
				 0,
				 lengthInSamps);
	sampleRate = reader->sampleRate;


	const auto normGain = [&reader](){
		std::array<juce::Range<float> , 1> normalizationRange;
		reader->readMaxLevels(0, reader->lengthInSamples, &normalizationRange[0], 1);
		const auto min = normalizationRange[0].getStart();
		const auto max = normalizationRange[0].getEnd();

		if (auto const normVal = std::max(std::abs(min), std::abs(max)); normVal > 0.f){
			return 1.f / normVal;
		}
		std::cerr << "either the sample is digital silence, or something's gone wrong\n";
		return 1.f;
	}();

	sampleBuffer.applyGain(normGain);
	waveformHash = computeHash(sampleBuffer);
	return true;
}

juce::String computeHash(const juce::AudioBuffer<float> &bufferToHash)
{
	if (bufferToHash.getNumSamples() == 0) {
		return {};
	}

	// Hash just based on channel 0 (consistent with analyzer)
	std::vector<float> audioData;
	audioData.reserve(bufferToHash.getNumSamples());

	const float* channelData = bufferToHash.getReadPointer(0);
	audioData.insert(audioData.end(), channelData, channelData + bufferToHash.getNumSamples());

	return hashAudioData(audioData);
}

void SampleManagementGuts::clear()
{
	sampleBuffer.clear();
	waveformHash = juce::String();
}

}	// namespace nvs::util