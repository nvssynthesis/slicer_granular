#include "PluginProcessor.h"
#include "PluginEditor.h"
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include "fmt/core.h"
#endif
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
logFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile).
		  getSiblingFile("log.txt"))
, fileLogger(logFile, "slicer_granular logging")
, apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
, granular_synth_juce(lastSampleRate, sampleBuffer,
					  lastFileSampleRate, num_voices)

{
	juce::Logger::setCurrentLogger (&fileLogger);
	granular_synth_juce.setLogger([this](const juce::String& message)
	{
		if (fileLogger.getCurrentLogger())
			fileLogger.logMessage(message);
	});
	formatManager.registerBasicFormats();
	granular_synth_juce.setNoteStealingEnabled (true);
}

Slicer_granularAudioProcessor::~Slicer_granularAudioProcessor()
{
	fileLogger.trimFileSize(logFile , 64 * 1024);
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
	lastSampleRate = sampleRate;
	granular_synth_juce.setCurrentPlaybackSampleRate (sampleRate);
	for (int i = 0; i < granular_synth_juce.getNumVoices(); i++)
	{
		if (auto voice = dynamic_cast<GranularVoice*>(granular_synth_juce.getVoice(i)))
		{
			voice->prepareToPlay (sampleRate, samplesPerBlock);
		}
	}
}

void Slicer_granularAudioProcessor::releaseResources(){}

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

void Slicer_granularAudioProcessor::writeToLog(juce::String const &s) {
	fileLogger.writeToLog (s);
}

void Slicer_granularAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	// Store parameters in memory block
	auto state = apvts.copyState();
	std::unique_ptr<juce::XmlElement> xml (state.createXml());
	fileLogger.logMessage("getStateInformation");
	
	copyXmlToBinary (*xml, destData);
}

void Slicer_granularAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	// Restore parameters from memory block
	std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
	fileLogger.logMessage("setStateInformation");
	
	if (xmlState.get() != nullptr){
		if (xmlState->hasTagName (apvts.state.getType())){
			apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
			
			juce::Value sampleFilePath = apvts.state.getPropertyAsValue(audioFilePathValueTreeStateIdentifier, nullptr, true);
			juce::File const sampleFile = juce::File(sampleFilePath.toString());
			loadAudioFile(sampleFile, true);
		}
	}
}
void Slicer_granularAudioProcessor::loadAudioFilesFolder(juce::File const folder){
	fileLogger.logMessage("loadAudioFilesFolder is not implemented!");
}
void Slicer_granularAudioProcessor::readInAudioFileToBuffer(juce::File const f){
	fileLogger.logMessage("                                          ...reading file" + f.getFullPathName());

	juce::AudioFormatReader *reader = formatManager.createReaderFor(f);
	if (!reader){
		std::cerr << "could not read file: " << f.getFileName() << "\n";
		return;
	}
	
	lastFileSampleRate = reader->sampleRate;
	
	std::array<juce::Range<float> , 1> normalizationRange;
	reader->readMaxLevels(0, reader->lengthInSamples, &normalizationRange[0], 1);
	auto min = normalizationRange[0].getStart();
	auto max = normalizationRange[0].getEnd();
	
	auto normVal = std::max(std::abs(min), std::abs(max));
	if (normVal > 0.f){
		normalizationValue = 1.f / normVal;
	} else {
		std::cerr << "either the sample is digital silence, or something's gone wrong\n";
		normalizationValue = 1.f;
	}
	int lengthInSamps = static_cast<int>(reader->lengthInSamples);
	sampleBuffer.setSize(reader->numChannels, lengthInSamps);
	
	reader->read(sampleBuffer.getArrayOfWritePointers(),	// float *const *destChannels
				 reader->numChannels,		// int numDestChannels
				 0,							// int64 startSampleInSource
				 lengthInSamps);	// int numSamplesToRead
	delete reader;
	fileLogger.logMessage("                                          ...file read successful");

}
void Slicer_granularAudioProcessor::loadAudioFileAsync(juce::File const file, bool notifyEditor)
{
	auto* loader = new AudioFileLoaderThread(*this, file, notifyEditor);
	loader->startThread(); // Starts the thread
}
void Slicer_granularAudioProcessor::loadAudioFile(juce::File const f, bool notifyEditor){
	fileLogger.logMessage("Slicer_granularAudioProcessor::loadAudioFile");

	const juce::SpinLock::ScopedLockType lock(audioBlockLock);
	fileLogger.logMessage("                                          ...locked");

	readInAudioFileToBuffer(f);
	granular_synth_juce.setAudioBlock(sampleBuffer);
	{
		juce::Value sampleFilePathValue = apvts.state.getPropertyAsValue(audioFilePathValueTreeStateIdentifier, nullptr, true);
		sampleFilePathValue.setValue(f.getFullPathName());
		sampleFilePath = sampleFilePathValue.toString();
		apvts.state.setProperty(audioFilePathValueTreeStateIdentifier, sampleFilePathValue, nullptr);
	}
	if (notifyEditor){
		fileLogger.logMessage("Processor: sending change message from loadAudioFile");
		juce::MessageManager::callAsync([this]() { sendChangeMessage(); });
	}
}
juce::String Slicer_granularAudioProcessor::getSampleFilePath() const {
	return sampleFilePath;
}
juce::AudioProcessorValueTreeState &Slicer_granularAudioProcessor::getAPVTS(){
	return apvts;
}
juce::AudioFormatManager &Slicer_granularAudioProcessor::getAudioFormatManager(){
	return formatManager;
}
void Slicer_granularAudioProcessor::logIfNaNOrInf(juce::AudioBuffer<float> buffer){
	float rms = 0.f;
	for (auto ch = 0; ch < buffer.getNumChannels(); ++ch){
		rms += buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
	}
	if (rms != rms) {
		writeToLog("processBlock:						rms was NaN");
	}
	if (std::isinf(rms)) {
		writeToLog("processBlock:						rms was Inf");
	}
}

void Slicer_granularAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i){
		buffer.clear (i, 0, buffer.getNumSamples());
	}
	
	float* const*  wp = sampleBuffer.getArrayOfWritePointers();
	auto const numSampChans = sampleBuffer.getNumChannels();
	for (int i = 0; i < numSampChans; ++i){
		if (!wp[i]){
			writeToLog("processBlock:        sampleBuffer null; exiting early.");
			return;
		}
	}
	if (sampleBuffer.getNumSamples() == 0){
		writeToLog("processBlock:        sampleBuffer has length 0; exiting early.");
		return;
	}
	if (numSampChans == 0){
		writeToLog("processBlock:        sampleBuffer has no channels; exiting early.");
		return;
	}
	
	const juce::SpinLock::ScopedTryLockType lock(audioBlockLock);
	if (!lock.isLocked()){
		writeToLog("processBlock:        lock was not locked; exiting early.");
		return;
	}

	granular_synth_juce.granularMainParamSet<0, num_voices>(apvts);	// this just sets the params internal to the granular synth (effectively a voice)
	granular_synth_juce.envelopeParamSet<0, num_voices>(apvts);
	granular_synth_juce.renderNextBlock(buffer,
						  midiMessages,
						  0,
						  buffer.getNumSamples());
	logIfNaNOrInf(buffer);
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


juce::AudioProcessorValueTreeState::ParameterLayout Slicer_granularAudioProcessor::createParameterLayout(){
	fileLogger.logMessage("createParamLayout\n");
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	auto mainGranularParams = std::make_unique<juce::AudioProcessorParameterGroup>("Gran", "MainGranularParams", "|");
	
	auto stringFromValue = [&](float value, int maximumStringLength){
		return juce::String (value, 2);	//getNumDecimalPlacesToDisplay()
	};
	
	auto a = [&](params_e p){
		auto tup = paramMap.at(p);
		auto name = std::get<static_cast<size_t>(param_elem_e::name)>(tup);
		juce::NormalisableRange<float> range = getNormalizableRange<float>(p);
		auto defaultVal = getParamDefault(p);
		
		return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(name,  1),	//	parameterID
												   name,		// parameterName
												   range,		// NormalizableRange
												   defaultVal,	// defaultValue
												   name,		// parameterLabel
												   juce::AudioProcessorParameter::genericParameter,	// category
												   stringFromValue,		// stringFromValue
												   nullptr);	// valueFromString
	};
	
	for (size_t i = 0; i < static_cast<size_t>(params_e::count_main_granular_params); ++i){
		params_e param = static_cast<params_e>(i);
		mainGranularParams->addChild(a(param));
	}
	layout.add(std::move(mainGranularParams));
	
	auto envelopeParams = std::make_unique<juce::AudioProcessorParameterGroup>("Env", "EnvelopeParams", "|");
	
	for (size_t i = static_cast<size_t>(params_e::count_main_granular_params) + 1;
		 i < static_cast<size_t>(params_e::count_envelope_params);
		 ++i){
		params_e param = static_cast<params_e>(i);
		envelopeParams->addChild(a(param));
	}
	layout.add(std::move(envelopeParams));
	return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Slicer_granularAudioProcessor();
}
