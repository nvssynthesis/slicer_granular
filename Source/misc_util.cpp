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
	audioHash = computeHash(sampleBuffer);
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
	audioHash = juce::String();
}


	juce::String sanitizeXmlName(const juce::String& name)
{
	// Replace invalid characters with underscores
	juce::String sanitized;

	for (int i = 0; i < name.length(); ++i)
	{
		auto c = name[i];

		// First character must be letter or underscore
		if (i == 0)
		{
			if (juce::CharacterFunctions::isLetter(c) || c == '_')
				sanitized += c;
			else
				sanitized += '_';
		}
		else
		{
			// Subsequent chars can be letter, digit, underscore, hyphen, or period
			if (juce::CharacterFunctions::isLetterOrDigit(c) || c == '_' || c == '-' || c == '.')
				sanitized += c;
			else
				sanitized += '_';
		}
	}

	// Ensure we have at least something
	if (sanitized.isEmpty())
		sanitized = "_";

	return sanitized;
}

juce::String valueTreeToXmlStringSafe(const juce::ValueTree& tree)
{
    // Create a deep copy to avoid modifying the original
    juce::ValueTree safeCopy = tree.createCopy();

    // Lambda to recursively process the tree
    std::function<void(juce::ValueTree&)> processTree = [&](juce::ValueTree& vt)
    {
        // Collect properties to rename/replace
        juce::Array<juce::Identifier> propsToRemove;
        juce::Array<std::pair<juce::Identifier, juce::var>> propsToAdd;

        // Process all properties
        for (int i = 0; i < vt.getNumProperties(); ++i)
        {
            auto propName = vt.getPropertyName(i);
            auto value = vt.getProperty(propName);
            juce::String propNameStr = propName.toString();

            // Check if property name needs sanitization
            juce::String sanitizedName = sanitizeXmlName(propNameStr);
            bool nameNeedsSanitizing = (sanitizedName != propNameStr);

            // Check if it's an array
            if (auto* arr = value.getArray())
            {
                // Replace array with descriptive string
                juce::String arrayDesc = "array of length " + juce::String(arr->size());

                // Optionally include type info if array is not empty
                if (arr->size() > 0)
                {
                    auto firstElement = (*arr)[0];
                    if (firstElement.isInt())
                        arrayDesc += " (int)";
                    else if (firstElement.isDouble())
                        arrayDesc += " (double)";
                    else if (firstElement.isString())
                        arrayDesc += " (string)";
                    else if (firstElement.isBool())
                        arrayDesc += " (bool)";
                    else
                        arrayDesc += " (mixed/other)";
                }

                if (nameNeedsSanitizing)
                {
                    propsToRemove.add(propName);
                    propsToAdd.add({juce::Identifier(sanitizedName), arrayDesc});
                }
                else
                {
                    vt.setProperty(propName, arrayDesc, nullptr);
                }
            }
            else if (nameNeedsSanitizing)
            {
                // Property name needs sanitizing but value is fine
                propsToRemove.add(propName);
                propsToAdd.add({juce::Identifier(sanitizedName), value});
            }
        }

        // Apply property name changes
        for (auto& prop : propsToRemove)
            vt.removeProperty(prop, nullptr);

        for (auto& [id, val] : propsToAdd)
            vt.setProperty(id, val, nullptr);

        // Recursively process all children
        for (auto child : vt)
            processTree(child);
    };

    processTree(safeCopy);

    return safeCopy.toXmlString();
}

}	// namespace nvs::util
