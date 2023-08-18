/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/
/// TODO:
//		organize files so that gen only copy is in proper folder, this project refs that file
//		button for file load
//		pan main control should be overall pan tendency; pan rand should be what current pan knob does
// 		implement other randomess
//		output gain
//		THEN THE BIG TSARA STUFF

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Slicer_granularAudioProcessor::Slicer_granularAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
, ess_hold(ess_init)
, gen_granular(lastSampleRate, audioBuffersChannels.getActiveSpanRef())
, logFile("/Users/nicholassolem/development/slicer_granular/Builds/MacOSX/build/Debug/log.txt")
, fileLogger(logFile, "hello")
{
	juce::Logger::setCurrentLogger (&fileLogger);
	formatManager.registerBasicFormats();
}

Slicer_granularAudioProcessor::~Slicer_granularAudioProcessor()
{
	juce::Logger::setCurrentLogger (nullptr);
	
	formatManager.clearFormats();
}

//==============================================================================
const juce::String Slicer_granularAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Slicer_granularAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Slicer_granularAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Slicer_granularAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Slicer_granularAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Slicer_granularAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Slicer_granularAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Slicer_granularAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Slicer_granularAudioProcessor::getProgramName (int index)
{
    return {};
}

void Slicer_granularAudioProcessor::changeProgramName (int index, const juce::String& newName){}

//==============================================================================
void Slicer_granularAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
	lastSampleRate = static_cast<float>(sampleRate);
	lastSamplesPerBlock = samplesPerBlock;
//	nvs::gran::loadFileIntoWaveHolder(wave_holder, "/Users/nicholassolem/development/audio for analysis/CBF_bamboo_flute_mono.wav", ess_hold.factory, lastSampleRate);
//	gran_synth.loadOnsets();
}

void Slicer_granularAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Slicer_granularAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Slicer_granularAudioProcessor::writeToLog(std::string const s){
	fileLogger.writeToLog (s);
}
void Slicer_granularAudioProcessor::loadAudioFile(juce::File const f){
	juce::AudioFormatReader *reader = formatManager.createReaderFor(f);
	
	int newLength = static_cast<int>(reader->lengthInSamples);
	
	std::array<juce::Range<float> , 1> normalizationRange;
	reader->readMaxLevels(0, reader->lengthInSamples, &normalizationRange[0], 1);
	
//#pragma message ("make use of normalization")
	auto min = normalizationRange[0].getStart();
	auto max = normalizationRange[0].getEnd();
	
	
	std::array<float * const, 2> ptrsToWriteTo = audioBuffersChannels.prepareForWrite(newLength, reader->numChannels);
	
	reader->read(&ptrsToWriteTo[0],	// float *const *destChannels
				 1, 	//reader->numChannels,		// int numDestChannels
				 0,		// int64 startSampleInSource
				 newLength);	// int numSamplesToRead
	
	audioBuffersChannels.updateActive();
	delete reader;
}


using granMembrSetFunc = void(nvs::gran::genGranPoly1::*) (float);
typedef StaticMap<params_e, granMembrSetFunc, static_cast<size_t>(params_e::count)>::iterator iter;
typedef StaticMap<params_e, granMembrSetFunc, static_cast<size_t>(params_e::count)>::const_iterator citer;

#if (STATIC_MAP | FROZEN_MAP)
template <auto Start, auto End>
constexpr void Slicer_granularAudioProcessor::paramSet(){
	float tmp;

	if constexpr (Start < End){
		constexpr params_e p = static_cast<params_e>(Start);
		granMembrSetFunc func = paramSetterMap.at(p);
		tmp = *apvts.getRawParameterValue(getParamElement<p, param_elem_e::name>());
		float *last = lastParamsMap.at(p);
		if (tmp != *last){
			*last = tmp;
			granMembrSetFunc setFunc = paramSetterMap.at(p);
			(gen_granular.*setFunc)(tmp);	// could replace with std::invoke
		}
		
		paramSet<Start + 1, End>();
	}
}
#endif

void Slicer_granularAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
	
	
#if (STATIC_MAP | FROZEN_MAP)
	paramSet<0, static_cast<int>(params_e::count)>();
#else
	float tmp;
	// normally we'd have the synth voice as a juce synth voice and have to dynamic cast before setting its params
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::transpose, param_elem_e::name>());
	if (lastTranspose != tmp){
		lastTranspose = tmp;
		
//		constexpr float semitoneRatio = 1.059463094359295f;
//		tmp = pow(semitoneRatio, tmp);
		
		gen_granular.setTranspose(tmp);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::position, param_elem_e::name>());
	if (lastPosition != tmp){
		lastPosition = tmp;
		gen_granular.setPosition(lastPosition);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::speed, param_elem_e::name>());
	if	(lastSpeed != tmp){
		lastSpeed = tmp;
		gen_granular.setSpeed(lastSpeed);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::duration, param_elem_e::name>());
	if (lastDuration != tmp){
		lastDuration = tmp;
		gen_granular.setDuration(lastDuration);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::skew, param_elem_e::name>());
	if (lastSkew != tmp){
		lastSkew = tmp;
		gen_granular.setSkew(lastSkew);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::pan, param_elem_e::name>());
	if (lastPan != tmp){
		lastPan = tmp;
		gen_granular.setPan(lastPan);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::pos_randomness, param_elem_e::name>());
	if (lastPositionRand != tmp){
		lastPositionRand = tmp;
		gen_granular.setPositionRandomness(lastPositionRand);
	}
	// lastSpeedRand
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::speed_randomness, param_elem_e::name>());
	if (lastSpeedRand != tmp){
		lastSpeedRand = tmp;
		gen_granular.setSpeedRandomness(lastSpeedRand);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::dur_randomness, param_elem_e::name>());
	if (lastDurationRand != tmp){
		lastDurationRand = tmp;
		gen_granular.setDurationRandomness(lastDurationRand);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::skew_randomness, param_elem_e::name>());
	if (lastSkewRand != tmp){
		lastSkewRand = tmp;
		gen_granular.setSkewRandomness(lastSkewRand);
	}
	tmp = *apvts.getRawParameterValue(getParamElement<params_e::pan_randomness, param_elem_e::name>());
	if (lastPanRand != tmp){
		lastPanRand = tmp;
		gen_granular.setPanRandomness(lastPanRand);
	}
#endif
	float trigger = static_cast<float>(triggerValFromEditor);
	for (const juce::MidiMessageMetadata metadata : midiMessages){
		if (metadata.numBytes == 3){
            fileLogger.writeToLog (metadata.getMessage().getDescription());
			//trigger = 1.f;
		}
	}
//	editorInformant.reset();
	for (auto samp = 0; samp < buffer.getNumSamples(); ++samp){
		std::array<float, 2> output = gen_granular(trigger);

		rms.accumulate(output[0] + output[1]);
//		trigger = 0.f;
		for (int channel = 0; channel < totalNumOutputChannels; ++channel)
		{
			output[channel] = juce::jlimit(-1.f, 1.f, output[channel]);

			auto* channelData = buffer.getWritePointer (channel);
			*(channelData + samp) = output[channel];
		}
	}
	const auto rms_val = rms.query();
	rmsInformant.val = rms_val;
	rmsWAinformant.val = weightAvg(rms_val);
}

//==============================================================================
bool Slicer_granularAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Slicer_granularAudioProcessor::createEditor()
{
    return new Slicer_granularAudioProcessorEditor (*this);
}

//==============================================================================
void Slicer_granularAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Slicer_granularAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout Slicer_granularAudioProcessor::createParameterLayout(){
	std::cout << "createParamLayout\n";
	
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	
	auto stringFromValue = [&](float value, int maximumStringLength){
		return juce::String (value, 4);	//getNumDecimalPlacesToDisplay()
	};
	
	auto a = [&](params_e p){
		auto tup = paramMap.at(p);
		auto name = std::get<static_cast<size_t>(param_elem_e::name)>(tup);
		// (ValueType rangeStart, ValueType rangeEnd, ValueType intervalValue, ValueType skewFactor, bool useSymmetricSkew=false)
		juce::NormalisableRange<float> range = getNormalizableRange<float>(p);
		auto defaultVal = std::get<static_cast<size_t>(param_elem_e::defaultVal)>(tup);
		
		return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(name,  1),	//	parameterID
												   name,		// parameterName
												   range,		// NormalizableRange
												   defaultVal,	// defaultValue
												   name,		// parameterLabel
												   juce::AudioProcessorParameter::genericParameter,	// category
												   stringFromValue,		// stringFromValue
												   nullptr);	// valueFromString
	};
	
	for (size_t i = 0; i < static_cast<size_t>(params_e::count); ++i){
		params_e param = static_cast<params_e>(i);
		/*if (!isMainParam(param)){
			std::cout << getParamName(param) << " skipped \n";
			break;
		}*/
		layout.add(a(param));
	}
	
	return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Slicer_granularAudioProcessor();
}
