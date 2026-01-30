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

}	// namespace nvs::util