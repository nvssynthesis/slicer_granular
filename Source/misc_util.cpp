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
void nvs::util::LoggingGuts::logIfNaNOrInf(juce::AudioBuffer<float> buffer){
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
