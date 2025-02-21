/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <filesystem>
#include <memory>

#include <iostream>


using namespace juce;

/*
Each line of the serial message can be one of the following 3 message types :

-note_on :
    -start playing a note with the given pitch
    - format : `note_on <pitch>`
    - example: `note_on 65`
    - note_off:
-stop playing a note
- format : `note_off <pitch>`
- example: `note_off 65`
- control:
-format : `control` followed by 6 integers between 0~1023, separated by a single space.The 6 numbers  represent following acoustic features respectively :
1. intensity
1. roughness
1. pitch variance
1. bow position
1. resonance
1. sharpness
- example: `control 100 200 300 400 500 600`
*/


void NetworkThread::run()
{
    while (!threadShouldExit())
    {
		continue;
        auto response = URL(p->getState("ServerUrl") + "/serial").readEntireTextStream();
        // trim '"' from response
        response = response.substring(1, response.length() - 1);
        //juce::Logger::writeToLog(response);
        // skip if response is empty
        if (response.startsWith("control ")) {
            int values[8] = { 0 };
			response = response.fromFirstOccurrenceOf("control ", false, false);
			int i = 0;
            for (auto character : response) {
                if (character == ' ') {
                    i++;
                }
                else {
                    values[i] = values[i] * 10 + (character - '0');
                }
            }
            juce::Logger::writeToLog("Received: " + juce::String(values[0]) + " " + juce::String(values[1]) + " " + juce::String(values[2]) + " " + juce::String(values[3]) + " " + juce::String(values[4]) + " " + juce::String(values[5]) + " " + juce::String(values[6]) + " " + juce::String(values[7]));
            auto control = (*mfmControls)["__dynamic__"];
            control->intensity[0] = values[0] / 1023.0;
            control->density[0] = values[1] / 1023.0;
            control->pitch[0] = (values[2] / 1023.0 - 0.5) * 5;
            control->hue[0] = values[3] / 1023.0 * 140;
            control->saturation[0] = values[4] / 1023.0;
            control->value[0] = values[5] / 1023.0;
		}
		else if (response.startsWith("note_on ")) {
			int pitch = response.fromFirstOccurrenceOf("note_on ", false, false).getIntValue();
			juce::Logger::writeToLog("Received: note_on " + juce::String(pitch));
			if (pitch >= 0 && pitch < 128)
				p->internalMidiMessages.push(MidiMessage::noteOn(1, pitch, 1.0f));
		}
		else if (response.startsWith("note_off ")) {
			int pitch = response.fromFirstOccurrenceOf("note_off ", false, false).getIntValue();
			juce::Logger::writeToLog("Received: note_off " + juce::String(pitch));
			if (pitch >= 0 && pitch < 128)
			    p->internalMidiMessages.push(MidiMessage::noteOff(1, pitch, 0.0f));
		}
		wait(80); // hope this is not too fast
    }
}

//==============================================================================
PhysicsBasedSynthAudioProcessor::PhysicsBasedSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ) 
#endif

    ,valueTree(*this, nullptr, "Parameters", createParameters()),
	networkThread(&mfmControls, this)
{
    mySynth.clearVoices();

    for (int i = 0; i < 10; i++)
    {
        auto voice = new SynthVoice();
        voice->setValueTree(valueTree);
        mySynth.addVoice(voice);
    }


    mySynth.clearSounds();
    mySynth.addSound(new SynthSound());
}

PhysicsBasedSynthAudioProcessor::~PhysicsBasedSynthAudioProcessor()
{
	networkThread.stopThread(1000);
}

//==============================================================================
const juce::String PhysicsBasedSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PhysicsBasedSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PhysicsBasedSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PhysicsBasedSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PhysicsBasedSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PhysicsBasedSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PhysicsBasedSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PhysicsBasedSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PhysicsBasedSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void PhysicsBasedSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PhysicsBasedSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    mySynth.setCurrentPlaybackSampleRate(sampleRate);

 //   dsp::ProcessSpec spec;
 //   spec.sampleRate = sampleRate;
 //   spec.maximumBlockSize = samplesPerBlock;
 //   spec.numChannels = getTotalNumOutputChannels();
 //   convolution.reset(); 
 //   convolution.prepare(spec); 
	//convolution.loadImpulseResponse(
 //       piano_ir,
 //       sizeof(char) * piano_ir_len,
 //       juce::dsp::Convolution::Stereo::yes,
	//	juce::dsp::Convolution::Trim::no,
 //       0
 //   );
 //   dryBuffer.setSize(spec.numChannels, spec.maximumBlockSize);

	for (int i = 0; i < mySynth.getNumVoices(); i++)
	{
		if (auto synthVoice = dynamic_cast<SynthVoice*>(mySynth.getVoice(i)))
		{
			synthVoice->prepareToPlay(&mfmParams, &mfmControls, &channelToImage, currentNoteChannel);
		}
	}



    auto dynamicControl = std::make_shared<MFMControl>(1);
	dynamicControl->intensity[0] = 0.8;
	dynamicControl->pitch[0] = 0;
	dynamicControl->density[0] = 0.8;
	dynamicControl->hue[0] = 0.5;
	dynamicControl->saturation[0] = 0.5;
	dynamicControl->value[0] = 0.5;
	mfmControls["__dynamic__"] = dynamicControl;
	channelToImage[1] = "__dynamic__";

}

void PhysicsBasedSynthAudioProcessor::loadImages()
{
    // load all notation images
    std::vector<juce::String> notationPaths;
    auto notationDir = getState("ImagesDirectory").toStdString();
    for (const auto& entry : std::filesystem::directory_iterator(notationDir))
    {
        if (entry.path().extension() == ".png")
        {
            notationPaths.push_back(entry.path().string());
        }
    }
    for (juce::String path : notationPaths)
    {
        auto name = File(path).getFileNameWithoutExtension();

        addNotation(name, juce::File(path));
    }
}

void PhysicsBasedSynthAudioProcessor::loadParams()
{
    loadMfmParamsFromFolder(getState("TableDirectory"));
}

void PhysicsBasedSynthAudioProcessor::startNetworkThread()
{
	if (!networkThread.isThreadRunning())
	{
        networkThread.startThread();
	}
}

void PhysicsBasedSynthAudioProcessor::addNotation(juce::String name, juce::File image) {
    images[name] = ImageFileFormat::loadFrom(image);
	channelToImage[channelToImage.size() + 2] = name; // 1 is for the dynamic control
	mfmControls[name] = std::make_shared<MFMControl>(notationToControl(image, getState("ServerUrl")));
	imagesDataVersion++;
}



void PhysicsBasedSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PhysicsBasedSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void setParam(AudioProcessorValueTreeState& valueTree, String paramId, float value)
{
	valueTree.getParameter(paramId)->beginChangeGesture();
	valueTree.getParameter(paramId)->setValueNotifyingHost(value);
	valueTree.getParameter(paramId)->endChangeGesture();
}

void PhysicsBasedSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

	// merge internalMidiMessages and midiMessages
	while (!internalMidiMessages.empty())
	{
		midiMessages.addEvent(internalMidiMessages.front(), 0);
		internalMidiMessages.pop();
	}

    std::shared_ptr<MFMControl> control = mfmControls["__dynamic__"];
	MidiBuffer::Iterator it(midiMessages);
	MidiMessage message;
	int sampleNumber;
    while (it.getNextEvent(message, sampleNumber)) {

		// update lastMidiMessage

		lastMidiMessage = message.getDescription();


		if (message.isNoteOn()) {
            const int midiChannel = message.getChannel();
            const int midiNote = message.getNoteNumber();
			currentNoteChannel[midiNote] = midiChannel;
        }
        
		// control change 11, 75-79 are used for MFM
        if (message.isController()) {
            const int controller = message.getControllerNumber();
            const int value = message.getControllerValue();

            switch (controller) {
			case 11: // expression
                control->intensity[0] = value / 127.0;
				
				setParam(valueTree, "intensity", value / 127.0);
                break;
            case 75:
                control->density[0] = value / 127.0 - 0.5;
                
				setParam(valueTree, "roughness", value / 127.0 - 0.5);
                break;
            case 76:
                control->pitch[0] = (value / 127.0 - 0.5) * 8;
				
				setParam(valueTree, "pitchVariance", (value / 127.0 - 0.5) * 8);
                break;
            case 77:
                control->hue[0] = value / 127.0 * 140;
                
				setParam(valueTree, "bowPosition", value / 127.0 * 140);
                break;
            case 78:
                control->saturation[0] = value / 127.0;
                
				setParam(valueTree, "resonance", value / 127.0);
                break;
            case 79:
                control->value[0] = value / 127.0;
				
				setParam(valueTree, "sharpness", value / 127.0);
                break;
            }
        }
    }



    mySynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    // dry signal
    
 //   dryBuffer.makeCopyOf(buffer, true);
 //   dsp::AudioBlock<float> block(buffer);
 //   dsp::ProcessContextReplacing<float> context(block);
 //   convolution.process(context);

 //   float wet = valueTree.getRawParameterValue("wet_dry")->load();
	//float dry = 1 - wet;

	//// gain wet signal
	//context.getOutputBlock().multiplyBy(wet);

	//// gain dry signal
	//dryBuffer.applyGain(dry);

	//// add dry signal
	//context.getOutputBlock().add<float>(dryBuffer);
}

//==============================================================================
bool PhysicsBasedSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PhysicsBasedSynthAudioProcessor::createEditor()
{
    return new PhysicsBasedSynthAudioProcessorEditor (*this);
}

void PhysicsBasedSynthAudioProcessor::setState(juce::String name, juce::String value)
{
    valueTree.state.getOrCreateChildWithName("stringState", nullptr).setProperty(name, value, nullptr);
}

juce::String PhysicsBasedSynthAudioProcessor::getState(juce::String name)
{
	return valueTree.state.getOrCreateChildWithName("stringState", nullptr).getProperty(name, "");
}

//==============================================================================
void PhysicsBasedSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
	auto state = valueTree.copyState();
	std::unique_ptr<XmlElement> xml(state.createXml());
	copyXmlToBinary(*xml, destData);
}

void PhysicsBasedSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTree.state.getType()))
            valueTree.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void PhysicsBasedSynthAudioProcessor::loadMfmParamsFromFolder(juce::String path)
{
	mfmParams.clear();
	for (const auto& entry : std::filesystem::directory_iterator(path.toStdString()))
	{
		if (entry.path().extension() == ".npz")
		{
			mfmParams[std::stoi(entry.path().stem().string())] = std::make_unique<MFMParam>(entry.path().string());
		}
		juce::Logger::writeToLog("Loaded MFM params from " + path);
	}
    
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhysicsBasedSynthAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout PhysicsBasedSynthAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

	// general parameters
    // gain
	params.push_back(std::make_unique<AudioParameterFloat>("gain", "Gain", 0.0f, 5.0f, 1));
	params.push_back(std::make_unique<AudioParameterFloat>("wetDry", "Wet Dry", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<AudioParameterFloat>("attack", "Attack", 0.0f, 2.0f, 1.0f));
	params.push_back(std::make_unique<AudioParameterFloat>("loopStart", "Loop Start", 0.0f, 5.0f, 0.5f));
    params.push_back(std::make_unique<AudioParameterFloat>("loopEnd", "Loop End", 0.0f, 5.0f, 1.75f));

	// feature parameters
	params.push_back(std::make_unique<AudioParameterFloat>("intensity", "Intensity", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<AudioParameterFloat>("roughness", "Roughness", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<AudioParameterFloat>("pitchVariance", "Pitch Variance", -8.0f, 8.0f, 0.0f));
	params.push_back(std::make_unique<AudioParameterFloat>("bowPosition", "Bow Position", 0.0f, 140.0f, 70.0f));
	params.push_back(std::make_unique<AudioParameterFloat>("resonance", "Resonance", 0.0f, 1.0f, 0.5f));
	params.push_back(std::make_unique<AudioParameterFloat>("sharpness", "Sharpness", 0.0f, 1.0f, 0.5f));


    return { params.begin(), params.end() };
}
