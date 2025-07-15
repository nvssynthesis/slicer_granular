#include "SlicerGranularPluginProcessor.h"
#include "SlicerGranularPluginEditor.h"
#if defined(DEBUG_BUILD) | defined(DEBUG) | defined(_DEBUG)
#include "fmt/core.h"
#endif

//==============================================================================

#ifndef TSN
namespace nvs::util{}
nvs::util::LoggingGuts::LoggingGuts()
: logFile(juce::File::getSpecialLocation(juce::File::SpecialLocationType::currentApplicationFile).getSiblingFile("log.txt"))
, fileLogger(logFile, "slicer_granular logging")
{
	juce::Logger::setCurrentLogger (&fileLogger);
}
#endif

SlicerGranularAudioProcessor::SlicerGranularAudioProcessor()
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
	apvts(*this, nullptr, "PLUGIN_STATE", createParameterLayout())
,	presetManager(apvts)
{
	apvts.state.appendChild (juce::ValueTree ("Settings"), nullptr);

	juce::ValueTree presetVT = apvts.state.getOrCreateChildWithName("PresetInfo", nullptr);
	if (!presetVT.hasProperty("sampleFilePath")){
		presetVT.setProperty("sampleFilePath", "", nullptr);
	}
	if (!presetVT.hasProperty("author")){
		presetVT.setProperty("author", "", nullptr);
	}
}


//==============================================================================
const juce::String SlicerGranularAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SlicerGranularAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SlicerGranularAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SlicerGranularAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SlicerGranularAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SlicerGranularAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SlicerGranularAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SlicerGranularAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SlicerGranularAudioProcessor::getProgramName (int index)
{
    return {};
}

void SlicerGranularAudioProcessor::changeProgramName (int index, const juce::String& newName){}

//==============================================================================
void SlicerGranularAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
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

void SlicerGranularAudioProcessor::releaseResources(){}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SlicerGranularAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SlicerGranularAudioProcessor::writeToLog(juce::String const &s) {
	loggingGuts.fileLogger.writeToLog (s);
}

void SlicerGranularAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	std::unique_ptr<juce::XmlElement> xml (apvts.state.createXml());
	copyXmlToBinary (*xml, destData);
}

void SlicerGranularAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

	if (xmlState == nullptr || ! xmlState->hasTagName ("PLUGIN_STATE")){
		return;
	}
	
	juce::ValueTree root = juce::ValueTree::fromXml (*xmlState);

	if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid()){
		apvts.replaceState (params);
	}

	auto const settings = apvts.state.getChildWithName ("Settings");

	if (auto const presetInfo = apvts.state.getChildWithName ("PresetInfo"); presetInfo.isValid()) {
		auto path = presetInfo.getProperty ("sampleFilePath").toString();
		if (path.isNotEmpty()) {
			loadAudioFile ({ path }, true);
		}
	}
	writeToLog("Successfully replaced APVTS\n");
	
}
nvs::gran::GranularSynthSharedState const &SlicerGranularAudioProcessor::viewSynthSharedState(){
	jassert (_granularSynth != nullptr);
	return _granularSynth->viewSynthSharedState();
}

[[deprecated]] void SlicerGranularAudioProcessor::loadAudioFileAsync(juce::File const file, bool notifyEditor)
{
	auto* loader = new AudioFileLoaderThread(*this, file, notifyEditor);
	loader->startThread();
}

void SlicerGranularAudioProcessor::loadAudioFile(juce::File const f, bool notifyEditor){
	loggingGuts.fileLogger.logMessage("Slicer_granularAudioProcessor::loadAudioFile");

	const juce::SpinLock::ScopedLockType lock(audioBlockLock);
	loggingGuts.fileLogger.logMessage("                                          ...locked");

	readInAudioFileToBuffer(f);
	if (notifyEditor){
		loggingGuts.fileLogger.logMessage("Processor: sending change message from loadAudioFile");
		
		// Whether to use Async or not probably could use more testing. 
		juce::MessageManager::callAsync([this]() { sampleManagementGuts.sendChangeMessage(); });
//		sampleManagementGuts.sendChangeMessage();
	}
	writeToLog("slicer: loadAudioFile exiting");
}

void SlicerGranularAudioProcessor::readInAudioFileToBuffer(juce::File const f){
	juce::String const fullPath = f.getFullPathName();
	writeToLog("                                          ...reading file" + fullPath);
	
	sampleManagementGuts.loadAudioFile(f);
	auto sr = sampleManagementGuts.getSampleRate();
	
	apvts.state.getChildWithName("PresetInfo").setProperty("sampleFilePath", fullPath, nullptr);
	apvts.state.getChildWithName("PresetInfo").setProperty("sampleRate", sr, nullptr);
	
	writeToLog("                                          ...file read successful");
	
	_granularSynth->setAudioBlock(sampleManagementGuts.getSampleBuffer(), sr, fullPath.hash());	// maybe this could just go inside readInAudioFileToBuffer()
}
juce::String SlicerGranularAudioProcessor::getSampleFilePath() const {
	return apvts.state.getChildWithName("PresetInfo").getProperty("sampleFilePath");
}
juce::AudioProcessorValueTreeState &SlicerGranularAudioProcessor::getAPVTS(){
	return apvts;
}
juce::AudioFormatManager &SlicerGranularAudioProcessor::getAudioFormatManager(){
	return sampleManagementGuts.getFormatManager();
}

void SlicerGranularAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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
	
	if (sampleManagementGuts.getLength() == 0){
		return;
	}
	if (sampleManagementGuts.getNumChannels() == 0){
		return;
	}
	
	
	_granularSynth->renderNextBlock(buffer,
						  midiMessages,
						  0,
						  buffer.getNumSamples());
	
	for (int i=0; i < buffer.getNumChannels(); ++i){
		auto *wp = buffer.getWritePointer(i);
		for (int j = 0; j < buffer.getNumSamples(); ++j){
			wp[j] = juce::jlimit(-1.f, 1.f, wp[j]);
		}
	}
	
	std::vector<nvs::gran::GrainDescription> descriptions = _granularSynth->getGrainDescriptions();
	writeGrainDescriptionData(descriptions);
	
	loggingGuts.logIfNaNOrInf(buffer);
}

void SlicerGranularAudioProcessor::writeGrainDescriptionData(const std::vector<nvs::gran::GrainDescription> &newData){
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
void SlicerGranularAudioProcessor::readGrainDescriptionData(std::vector<nvs::gran::GrainDescription> &outData){
	if (measuredGrainDescriptions.dataReady.load(std::memory_order_acquire)) {
		int activeBuffer = measuredGrainDescriptions.activeBufferIdx.load();
		const std::vector<nvs::gran::GrainDescription>& data = (activeBuffer == 0) ? measuredGrainDescriptions.data0 : measuredGrainDescriptions.data1;
		outData = data; // Copy data to output parameter
		measuredGrainDescriptions.dataReady.store(false, std::memory_order_release);
	}
}

//==============================================================================
bool SlicerGranularAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SlicerGranularAudioProcessor::createEditor()
{
    return new Slicer_granularAudioProcessorEditor (*this);
}

//==============================================================================
std::unique_ptr<juce::RangedAudioParameter> createJuceParameter(const nvs::param::ParameterDef& param) {
	if (param.getParameterType() == nvs::param::ParameterType::Float){
		
		nvs::param::ParameterDef::FloatParamElements floatParamElements = std::get<nvs::param::ParameterDef::FloatParamElements>(param.elementsVar);
		
		auto defaultStringFromValue = [floatParamElements, suffix = param.unitSuffix](float value, int) -> juce::String
		{
			return juce::String(value, floatParamElements.numDecimalPlaces) + suffix;
		};
		auto stringFromValueFn = floatParamElements.stringFromValue == nullptr ? defaultStringFromValue : floatParamElements.stringFromValue;
		
		auto defaultValueFromStringFn = [](juce::String const &s) -> float
		{
			return s.getFloatValue();
		};
		auto valueFromStringFn = floatParamElements.valueFromString == nullptr ? defaultValueFromStringFn : floatParamElements.valueFromString;
		
		
		return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{param.ID, 1},
														   param.displayName,
														   param.getFloatRange(),  // Uses template method for float version
														   floatParamElements.defaultVal,
														   juce::AudioParameterFloatAttributes()
														   .withStringFromValueFunction(stringFromValueFn)
														   .withValueFromStringFunction(valueFromStringFn)
														   );
	}
	jassert(param.getParameterType() == nvs::param::ParameterType::Choice);
	nvs::param::ParameterDef::ChoiceParamElements choiceParamElements = std::get<nvs::param::ParameterDef::ChoiceParamElements>(param.elementsVar);
	return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{param.ID, 1},
														param.displayName,
														choiceParamElements.choices,
														choiceParamElements.defaultChoiceIndex,
														juce::AudioParameterChoiceAttributes());
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
	using namespace nvs::param;
	
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	
	// organize parameters by main group
	std::map<juce::String, std::vector<ParameterDef>> groupedParams;
	for (const auto& param : ALL_PARAMETERS) {
		groupedParams[param.groupName].push_back(param);
	}
	// Create groups dynamically, handling nested sub-groups
	for (const auto& [groupName, params] : groupedParams) {
		auto mainGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
			groupName, groupName + "Params", "|");
		
		// check if any parameters in this group have sub-groups
		bool hasSubGroups = std::any_of(params.begin(), params.end(),
			[](const ParameterDef& p) { return p.hasSubGroup(); });
		
		if (hasSubGroups) {
			// organize by sub-groups
			std::map<juce::String, std::vector<ParameterDef>> subGroupedParams;
			
			for (const auto& param : params) {
				if (param.hasSubGroup()) {
					subGroupedParams[param.subGroupName].push_back(param);
				} else {
					// Parameters without sub-group go directly into main group
					mainGroup->addChild(createJuceParameter(param));
				}
			}
			
			// create sub-groups
			for (const auto& [subGroupName, subParams] : subGroupedParams) {
				auto subGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
					subGroupName, subGroupName + "SubParams", "|");
					
				for (const auto& param : subParams) {
					subGroup->addChild(createJuceParameter(param));
				}
				
				mainGroup->addChild(std::move(subGroup));
			}
		} else {
			// no sub-groups, add parameters directly
			for (const auto& param : params) {
				mainGroup->addChild(createJuceParameter(param));
			}
		}
		
		layout.add(std::move(mainGroup));
	}
	
	return layout;
}


#ifndef TSN
// this preprocessor definition should be defined in tsn_granular to prevent multiple definitions
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return SlicerGranularAudioProcessor::create<SlicerGranularAudioProcessor>();
}
#endif
