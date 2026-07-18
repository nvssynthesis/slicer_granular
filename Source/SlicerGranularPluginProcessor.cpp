#include "SlicerGranularPluginProcessor.h"
#include "SlicerGranularPluginEditor.h"
#include "./StringAxiom.h"
/*
 *TRY: 3POINT SLIDERS (2 THUMBS)
 *
 */
//==============================================================================

SlicerGranularAudioProcessor::SlicerGranularAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                       ),
#endif
	apvts(*this, nullptr, "nvs::axiom::PLUGIN_STATE", createParameterLayout())
,	presetManager(apvts)
{
	apvts.state.appendChild (ValueTree ("nvs::axiom::Settings"), nullptr);
	presetManager.addChangeListener(this);
}
SlicerGranularAudioProcessor::~SlicerGranularAudioProcessor() = default;

void SlicerGranularAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	_granularSynth->setCurrentPlaybackSampleRate (sampleRate);
	for (int i = 0; i < _granularSynth->getNumVoices(); i++)
	{
		if (auto voice = dynamic_cast<nvs::gran::GranularVoice *>(_granularSynth->getVoice(i)))
		{
			voice->prepareToPlay (sampleRate, samplesPerBlock);
		}
	}
}

void SlicerGranularAudioProcessor::writeToLog(String const &s) {
	loggingGuts.fileLogger.writeToLog (s);
}

void SlicerGranularAudioProcessor::getStateInformation (MemoryBlock& destData)
{
	std::unique_ptr<XmlElement> xml (apvts.state.createXml());
	copyXmlToBinary (*xml, destData);
}

void SlicerGranularAudioProcessor::loadStoredAudioFileAndUpdateState()
{
    if (auto fileInfo = apvts.state.getChildWithName(nvs::axiom::FileInfo);
        fileInfo.isValid())
    {
        if (auto const fp = fileInfo.getPropertyAsValue(nvs::axiom::sampleFilePath, nullptr).toString();
            fp.isNotEmpty())
        {
            loadAudioFileAndUpdateState(fp, true);
        }
    }
}

void SlicerGranularAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

	if (xmlState == nullptr || ! xmlState->hasTagName (nvs::axiom::PLUGIN_STATE)){
		return;
	}
	const ValueTree root = ValueTree::fromXml (*xmlState);
	apvts.replaceState (root);

	loadStoredAudioFileAndUpdateState();

	writeToLog("Successfully replaced APVTS\n");
}
void SlicerGranularAudioProcessor::changeListenerCallback (ChangeBroadcaster *source) {
	if (&presetManager == source){
		loadStoredAudioFileAndUpdateState();
	}
	else {
		writeToLog("SlicerGranularAudioProcessor::changeListenerCallback: unknown ChangeBroadcaster\n");
	}
}

//==============================================================================

nvs::gran::GranularSynthSharedState const &SlicerGranularAudioProcessor::viewSynthSharedState() const {
	jassert (_granularSynth != nullptr);
	return _granularSynth->viewSynthSharedState();
}

void SlicerGranularAudioProcessor::loadAudioFileAndUpdateState(const File f, const bool notifyEditor){
	loggingGuts.fileLogger.logMessage("Slicer_granularAudioProcessor::loadAudioFileAndUpdateState");

	const SpinLock::ScopedLockType lock(audioBlockLock);
	loggingGuts.fileLogger.logMessage("                                          ...locked");

#pragma message("we should find if the file was found and if not, give message")
	readIntoBufferAndUpdateState(f);
	if (notifyEditor){
		loggingGuts.fileLogger.logMessage("Processor: sending change message from loadAudioFileAndUpdateState");
		
		// Whether to use Async or not probably could use more testing. 
		MessageManager::callAsync([this]() { sampleManager.sendChangeMessage(); });
//		sampleManager.sendChangeMessage();
	}
	writeToLog("slicer: loadAudioFileAndUpdateState exiting");
}

void SlicerGranularAudioProcessor::readIntoBufferAndUpdateState(File const &f){
	String const fullPath = f.getFullPathName();
	writeToLog("                                          ...reading file" + fullPath);
	
	if (!sampleManager.loadAudioFile(f)) {
		writeToLog(fmt::format("readIntoBufferAndUpdateState: could not load file {}\n", fullPath.toStdString()));
	    return;
	}
	
	writeToLog("                                          ...file read successful");

    const auto sr = sampleManager.getSampleRate();
	_granularSynth->setAudioBuffer(sampleManager.getSampleBuffer(), sr, sampleManager.getWaveformHash().hashCode64()); // waveformHash is a String, but we need to rehash it to get int64
	
	auto fileInfo = apvts.state.getOrCreateChildWithName(nvs::axiom::FileInfo, nullptr);
	fileInfo.setProperty(nvs::axiom::sampleFilePath, fullPath, nullptr);
	fileInfo.setProperty(nvs::axiom::sampleRate, sr, nullptr);
	fileInfo.setProperty(nvs::axiom::audioHash, sampleManager.getWaveformHash(), nullptr);
}
String SlicerGranularAudioProcessor::getSampleFilePath() const {
	return apvts.state.getChildWithName(nvs::axiom::FileInfo).getProperty(nvs::axiom::sampleFilePath);
}
String SlicerGranularAudioProcessor::getAudioHash() const {
    if (const auto hashVar = apvts.state.getChildWithName(nvs::axiom::FileInfo).getProperty(nvs::axiom::audioHash);
        hashVar.isString())
    {
        const auto hashStr = hashVar.toString();
        return hashStr;
    }
    return "";
}
AudioProcessorValueTreeState &SlicerGranularAudioProcessor::getAPVTS(){
	return apvts;
}
AudioFormatManager &SlicerGranularAudioProcessor::getAudioFormatManager(){
	return sampleManager.getAudioFormatManager();
}

void SlicerGranularAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
	for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i){
		buffer.clear (i, 0, buffer.getNumSamples());
	}

    if (const SpinLock::ScopedTryLockType lock(audioBlockLock); !lock.isLocked())
    {
		writeToLog("processBlock: lock was not locked; exiting early.");
		return;
	}
	
	if ((sampleManager.getLength() == 0) || (sampleManager.getNumChannels() == 0)) {
		return;
	}
	
    _granularSynth->processBlock(buffer, midiMessages);

	for (int i = 0; i < buffer.getNumChannels(); ++i) {
        auto *wp = buffer.getWritePointer(i);
        for (int j = 0; j < buffer.getNumSamples(); ++j) {
            wp[j] = jlimit(-1.f, 1.f, wp[j]);
        }
    }

    const std::vector<nvs::gran::GrainDescription> descriptions = _granularSynth->getGrainDescriptions();
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
static std::unique_ptr<RangedAudioParameter> createJuceParameter(const nvs::param::ParameterDef& param) {
	if (param.getParameterType() == nvs::param::ParameterType::Float){
		
		nvs::param::ParameterDef::FloatParamElements floatParamElements = std::get<nvs::param::ParameterDef::FloatParamElements>(param.elementsVar);
		
		auto defaultStringFromValue = [floatParamElements, suffix = param.unitSuffix](float value, int) -> String
		{
			return String(value, floatParamElements.numDecimalPlaces) + suffix;
		};
		auto stringFromValueFn = floatParamElements.stringFromValue == nullptr ? defaultStringFromValue : floatParamElements.stringFromValue;
		
		auto defaultValueFromStringFn = [](String const &s) -> float
		{
			return s.getFloatValue();
		};
		auto valueFromStringFn = floatParamElements.valueFromString == nullptr ? defaultValueFromStringFn : floatParamElements.valueFromString;
		
		
		return std::make_unique<AudioParameterFloat>(ParameterID{param.ID, 1},
														   param.displayName,
														   param.getFloatRange(),  // Uses template method for float version
														   floatParamElements.defaultVal,
														   AudioParameterFloatAttributes()
														   .withStringFromValueFunction(stringFromValueFn)
														   .withValueFromStringFunction(valueFromStringFn)
														   );
	}
	jassert(param.getParameterType() == nvs::param::ParameterType::Choice);
	nvs::param::ParameterDef::ChoiceParamElements choiceParamElements = std::get<nvs::param::ParameterDef::ChoiceParamElements>(param.elementsVar);
	return std::make_unique<AudioParameterChoice>(ParameterID{param.ID, 1},
														param.displayName,
														choiceParamElements.choices,
														choiceParamElements.defaultChoiceIndex,
														AudioParameterChoiceAttributes());
}

AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
	using namespace nvs::param;
	
	AudioProcessorValueTreeState::ParameterLayout layout;
	
	// organize parameters by main group
	std::map<String, std::vector<ParameterDef>> groupedParams;
	for (const auto& param : ALL_PARAMETERS) {
		groupedParams[param.groupName].push_back(param);
	}
	// Create groups dynamically, handling nested sub-groups
	for (const auto& [groupName, params] : groupedParams) {
		auto mainGroup = std::make_unique<AudioProcessorParameterGroup>(
			groupName, groupName + "Params", "|");
		
		// check if any parameters in this group have sub-groups
		bool hasSubGroups = std::any_of(params.begin(), params.end(),
			[](const ParameterDef& p) { return p.hasSubGroup(); });
		
		if (hasSubGroups) {
			// organize by sub-groups
			std::map<String, std::vector<ParameterDef>> subGroupedParams;
			
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
				auto subGroup = std::make_unique<AudioProcessorParameterGroup>(
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

//=======================================================================================
//=======================================================================================
//=======================================================================================
// all of the below are essentially unmodified from standard AudioProcessor code
//=======================================================================================
//=======================================================================================
//=======================================================================================
const String SlicerGranularAudioProcessor::getName() const
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

void SlicerGranularAudioProcessor::setCurrentProgram ([[maybe_unused]] int index)
{}

const String SlicerGranularAudioProcessor::getProgramName ([[maybe_unused]] int index)
{
	return {};
}

void SlicerGranularAudioProcessor::changeProgramName ([[maybe_unused]] int index, [[maybe_unused]] const String& newName){}

//==============================================================================

void SlicerGranularAudioProcessor::releaseResources(){}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SlicerGranularAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
	ignoreUnused (layouts);
	return true;
  #else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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
//==============================================================================
bool SlicerGranularAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* SlicerGranularAudioProcessor::createEditor()
{
	return new Slicer_granularAudioProcessorEditor (*this);
}