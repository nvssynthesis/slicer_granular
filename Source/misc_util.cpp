/*
  ==============================================================================

    misc_util.cpp
    Created: 15 Jan 2025 4:49:27pm
    Author:  Nicholas Solem

  ==============================================================================
*/

#include "misc_util.h"


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
		std::cerr << "could not read file\n";
		jassertfalse;
		return false;
	}
	auto lengthInSamps = static_cast<int>(reader->lengthInSamples);
	
	sampleBuffer.setSize(reader->numChannels, lengthInSamps);
	reader->read(sampleBuffer.getArrayOfWritePointers(),
				 reader->numChannels,
				 0,
				 lengthInSamps);
	sampleRate = reader->sampleRate;

	
	auto normGain = [&reader](){
		std::array<juce::Range<float> , 1> normalizationRange;
		reader->readMaxLevels(0, reader->lengthInSamples, &normalizationRange[0], 1);
		auto min = normalizationRange[0].getStart();
		auto max = normalizationRange[0].getEnd();
		
		auto const normVal = std::max(std::abs(min), std::abs(max));	// abs max of neg and pos
		if (normVal > 0.f){
			return 1.f / normVal;
		}
		else {
			std::cerr << "either the sample is digital silence, or something's gone wrong\n";
			return 1.f;
		}
	}();
	
	sampleBuffer.applyGain(normGain);
	computeHash();
	return true;
}

void SampleManagementGuts::computeHash()
{
	if (sampleBuffer.getNumSamples() == 0) {
		hash = juce::String();
		return;
	}
	
	// Hash just based on channel 0 (consistent with analyzer)
	std::vector<float> audioData;
	audioData.reserve(sampleBuffer.getNumSamples());
	
	const float* channelData = sampleBuffer.getReadPointer(0);
	audioData.insert(audioData.end(), channelData, channelData + sampleBuffer.getNumSamples());
	
	hash = hashAudioData(audioData);
	std::cout << hash << '\n';
}

void SampleManagementGuts::clear()
{
	sampleBuffer.clear();
	hash = juce::String();
}


}	// namespace nvs::util
