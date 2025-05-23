#include "SlicerGranularPluginProcessor.h"
#include "SlicerGranularPluginEditor.h"
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include "fmt/core.h"
#endif
//==============================================================================

#ifndef TSN
nvs::util::LoggingGuts::LoggingGuts()
: logFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile).getSiblingFile("log.txt"))
, fileLogger(logFile, "slicer_granular logging")
{
	juce::Logger::setCurrentLogger (&fileLogger);
}
#endif

Slicer_granularAudioProcessor::Slicer_granularAudioProcessor(std::unique_ptr<GranularSynthesizer> granularSynth)
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
,	nonAutomatableState ("NonAutomatable")
,	_granularSynth(std::move(granularSynth))
{
	_granularSynth->setLogger([this](const juce::String& message)
	{
		if (loggingGuts.fileLogger.getCurrentLogger()){
			loggingGuts.fileLogger.logMessage(message);
		}
	});
	nonAutomatableState.appendChild (juce::ValueTree ("Settings"), nullptr);

	juce::ValueTree presetVT = nonAutomatableState.getOrCreateChildWithName("PresetInfo", nullptr);
	presetVT.setProperty("sampleFilePath", "", nullptr);
	presetVT.setProperty("author", "", nullptr);
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
	_granularSynth->setCurrentPlaybackSampleRate (sampleRate);
	for (int i = 0; i < _granularSynth->getNumVoices(); i++)
	{
		if (auto voice = dynamic_cast<GranularVoice*>(_granularSynth->getVoice(i)))
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
	loggingGuts.fileLogger.writeToLog (s);
}

void Slicer_granularAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	// Build a single root tree:
	juce::ValueTree root { "PluginState" };
	root.appendChild (apvts.copyState(),  nullptr);
	root.appendChild (nonAutomatableState,  nullptr);

	// XML → binary:
	std::unique_ptr<juce::XmlElement> xml (root.createXml());
	copyXmlToBinary (*xml, destData);
}

void Slicer_granularAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

	if (xmlState == nullptr || ! xmlState->hasTagName ("PluginState")){
		return;
	}
	
	juce::ValueTree root = juce::ValueTree::fromXml (*xmlState);

	if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid()){
		apvts.replaceState (params);
	}

	if (auto nonAuto = root.getChildWithName ("NonAutomatable"); nonAuto.isValid())
	{
		auto settings = nonAuto.getChildWithName ("Settings");

		if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid()){
			apvts.replaceState (params);
		}
		

		if (auto presetInfo = nonAutomatableState.getChildWithName ("PresetInfo"); presetInfo.isValid())
		{
			auto path = presetInfo.getProperty ("sampleFilePath").toString();
			if (path.isNotEmpty()) {
				loadAudioFile ({ path }, true);
			}
		}
		writeToLog("Successfully replaced APVTS and NonAutomatableState\n");
	}
}

void Slicer_granularAudioProcessor::readInAudioFileToBuffer(juce::File const f){
	loggingGuts.fileLogger.logMessage("                                          ...reading file" + f.getFullPathName());
	
	juce::AudioFormatReader *reader = sampleManagementGuts.formatManager.createReaderFor(f);
	if (!reader){
		std::cerr << "could not read file: " << f.getFullPathName() << "\n";
		return;
	}
	
	auto sr = reader->sampleRate;
	
	nonAutomatableState.getChildWithName("PresetInfo").setProperty("sampleFilePath", f.getFullPathName(), nullptr);
	nonAutomatableState.getChildWithName("PresetInfo").setProperty("sampleRate", sr, nullptr);
	
	std::array<juce::Range<float> , 1> normalizationRange;
	reader->readMaxLevels(0, reader->lengthInSamples, &normalizationRange[0], 1);
	auto min = normalizationRange[0].getStart();
	auto max = normalizationRange[0].getEnd();
	
	auto normVal = std::max(std::abs(min), std::abs(max));
	if (normVal > 0.f){
		sampleManagementGuts.normalizationValue = 1.f / normVal;
	} else {
		std::cerr << "either the sample is digital silence, or something's gone wrong\n";
		sampleManagementGuts.normalizationValue = 1.f;
	}
	int lengthInSamps = static_cast<int>(reader->lengthInSamples);
	sampleManagementGuts.sampleBuffer.setSize(reader->numChannels, lengthInSamps);
	
	reader->read(sampleManagementGuts.sampleBuffer.getArrayOfWritePointers(),	// float *const *destChannels
				 reader->numChannels,		// int numDestChannels
				 0,							// int64 startSampleInSource
				 lengthInSamps);	// int numSamplesToRead
	delete reader;
	loggingGuts.fileLogger.logMessage("                                          ...file read successful");

	_granularSynth->setAudioBlock(sampleManagementGuts.sampleBuffer, sr, f.getFullPathName().hash());	// maybe this could just go inside readInAudioFileToBuffer()
	nonAutomatableState.getChildWithName("PresetInfo").setProperty("sampleFilePath", f.getFullPathName(), nullptr);
}
void Slicer_granularAudioProcessor::loadAudioFileAsync(juce::File const file, bool notifyEditor)
{
	auto* loader = new AudioFileLoaderThread(*this, file, notifyEditor);
	loader->startThread(); // Starts the thread
}

void Slicer_granularAudioProcessor::loadAudioFile(juce::File const f, bool notifyEditor){
	loggingGuts.fileLogger.logMessage("Slicer_granularAudioProcessor::loadAudioFile");

	const juce::SpinLock::ScopedLockType lock(audioBlockLock);
	loggingGuts.fileLogger.logMessage("                                          ...locked");

	readInAudioFileToBuffer(f);
	if (notifyEditor){
		loggingGuts.fileLogger.logMessage("Processor: sending change message from loadAudioFile");
		
		// I do not recall why I used async for this, but there was a bug associated. I could not reproduce the bug easily either way, but I suspect it had to do with async.
		// The bug was something along the lines of doing another analysis after one has already been performed, maybe via loading a new file. I don't recall the symptom of the bug.
//		juce::MessageManager::callAsync([this]() { sampleManagementGuts.sendChangeMessage(); });
		sampleManagementGuts.sendChangeMessage();
	}
	writeToLog("slicer: loadAudioFile exiting");
}
juce::String Slicer_granularAudioProcessor::getSampleFilePath() const {
	return nonAutomatableState.getChildWithName("PresetInfo").getProperty("sampleFilePath");
}
juce::AudioProcessorValueTreeState &Slicer_granularAudioProcessor::getAPVTS(){
	return apvts;
}
juce::ValueTree &Slicer_granularAudioProcessor::getNonAutomatableState() {
	return nonAutomatableState;
}
juce::AudioFormatManager &Slicer_granularAudioProcessor::getAudioFormatManager(){
	return sampleManagementGuts.formatManager;
}

void Slicer_granularAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i){
		buffer.clear (i, 0, buffer.getNumSamples());
	}
	
	const juce::SpinLock::ScopedTryLockType lock(audioBlockLock);
	if (!lock.isLocked()){
		writeToLog("processBlock:        lock was not locked; exiting early.");
		return;
	}
	
	float* const*  wp = sampleManagementGuts.sampleBuffer.getArrayOfWritePointers();
	auto const numSampChans = sampleManagementGuts.sampleBuffer.getNumChannels();
	for (int i = 0; i < numSampChans; ++i){
		if (!wp[i]){
			writeToLog("processBlock:        sampleBuffer null; exiting early.");
			return;
		}
	}
	if (sampleManagementGuts.sampleBuffer.getNumSamples() == 0){
//		writeToLog("processBlock:        sampleBuffer has length 0; exiting early.");
		return;
	}
	if (numSampChans == 0){
//		writeToLog("processBlock:        sampleBuffer has no channels; exiting early.");
		return;
	}
	
	_granularSynth->granularMainParamSet<0, GranularSynthesizer::getNumVoices()>(apvts);	// this just sets the params internal to the granular synth (effectively a voice)
	_granularSynth->envelopeParamSet<0, GranularSynthesizer::getNumVoices()>(apvts);
	_granularSynth->scannerParamSet<0, GranularSynthesizer::getNumVoices()>(apvts);
	
	_granularSynth->renderNextBlock(buffer,
						  midiMessages,
						  0,
						  buffer.getNumSamples());
	
	std::vector<nvs::gran::GrainDescription> descriptions = _granularSynth->getGrainDescriptions();
	writeGrainDescriptionData(descriptions);
	
	loggingGuts.logIfNaNOrInf(buffer);
}

void Slicer_granularAudioProcessor::writeGrainDescriptionData(const std::vector<nvs::gran::GrainDescription> &newData){
	int inactiveBuffer = measuredGrainDescriptions.activeBufferIdx.load() == 0 ? 1 : 0; // flip the buffer index
	if (inactiveBuffer == 0){
		measuredGrainDescriptions.data0 = newData;
	}
	else {
		measuredGrainDescriptions.data1 = newData;
	}
	measuredGrainDescriptions.dataReady.store(true);
	measuredGrainDescriptions.activeBufferIdx.store(inactiveBuffer, std::memory_order_release);
	measuredGrainDescriptions.sendChangeMessage();
}
void Slicer_granularAudioProcessor::readGrainDescriptionData(std::vector<nvs::gran::GrainDescription> &outData){
	if (measuredGrainDescriptions.dataReady.load(std::memory_order_acquire)) {
		int activeBuffer = measuredGrainDescriptions.activeBufferIdx.load();
		const std::vector<nvs::gran::GrainDescription>& data = (activeBuffer == 0) ? measuredGrainDescriptions.data0 : measuredGrainDescriptions.data1;
		outData = data; // Copy data to output parameter
		measuredGrainDescriptions.dataReady.store(false, std::memory_order_release);
	}
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
	loggingGuts.fileLogger.logMessage("createParamLayout\n");
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	auto mainGranularParams = std::make_unique<juce::AudioProcessorParameterGroup>("Gran", "MainGranularParams", "|");
	
	auto stringFromValue = [&](float value, int maximumStringLength){
		return juce::String (value, 3);	//getNumDecimalPlacesToDisplay()
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
	
	auto scannerParams = std::make_unique<juce::AudioProcessorParameterGroup>("Scanner", "ScannerParams", "|");
	
	for (size_t i =
		 static_cast<size_t>(params_e::pos_scan_amount);
		 i < static_cast<size_t>(params_e::count_pos_scan_params);
		 ++i){
		params_e param = static_cast<params_e>(i);
		scannerParams->addChild(a(param));
	}
	layout.add(std::move(scannerParams));
	
#ifdef TSN
	auto navigationParams = std::make_unique<juce::AudioProcessorParameterGroup>("Navigation", "NavParams", "|");
	auto navigationSelectionParams = std::make_unique<juce::AudioProcessorParameterGroup>("Selection", "NavSelParams", "|");

	for (size_t i = static_cast<size_t>(params_e::nav_selection_sharpness);
		 i < static_cast<size_t>(params_e::count_nav_selection_params);
		 ++i){
		params_e param = static_cast<params_e>(i);
		navigationSelectionParams->addChild(a(param));
	}
	navigationParams->addChild(std::move(navigationSelectionParams));
	
	for (size_t i = static_cast<size_t>(params_e::nav_tendency_x);
		 i < static_cast<size_t>(params_e::count_nav_tendency_params);
		 ++i){
		params_e param = static_cast<params_e>(i);
		navigationParams->addChild(a(param));
	}
	for (size_t i = static_cast<size_t>(params_e::nav_lfo_2d_amount);
		 i < static_cast<size_t>(params_e::count_lfo_2d_params);
		 ++i){
		params_e param = static_cast<params_e>(i);
		navigationParams->addChild(a(param));
	}
	for (size_t i = static_cast<size_t>(params_e::nav_random_walk_step_size);
		 i < static_cast<size_t>(params_e::count_random_walk_params);
		 ++i){
		params_e param = static_cast<params_e>(i);
		navigationParams->addChild(a(param));
	}
	
	layout.add(std::move(navigationParams));
#endif
	
	return layout;
}

#ifndef TSN
// this preprocessor definition should be defined in tsn_granular to prevent multiple definitions
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Slicer_granularAudioProcessor(std::make_unique<GranularSynthesizer>());
}
#endif
