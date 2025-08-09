//
// Created by Nicholas Solem on 8/8/25.
//

#include "SlicerGranularPluginProcessor.h"

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return SlicerGranularAudioProcessor::create<SlicerGranularAudioProcessor>();
}