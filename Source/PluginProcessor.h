/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once 
 
#include <JuceHeader.h>
#include "SynthVoice.h"
#include "SynthSound.h"
#include "MFMParam.h"
#include "MFMControl.h"
class PhysicsBasedSynthAudioProcessor;
class NetworkThread : public juce::Thread
{
private:
    std::map<juce::String, std::shared_ptr<MFMControl>>* mfmControls;
	PhysicsBasedSynthAudioProcessor* p;
public:
	NetworkThread(std::map<juce::String, std::shared_ptr<MFMControl>>* mfmControls, PhysicsBasedSynthAudioProcessor* p)
        : Thread("Network Thread")
		, mfmControls(mfmControls),
		p(p)
    {
    }
    void run() override;
};


//==============================================================================
/**
*/
class PhysicsBasedSynthAudioProcessor  : public juce::AudioProcessor
{
public: 
    //==============================================================================
    PhysicsBasedSynthAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState valueTree;

    void addNotation(juce::String name, juce::File image);

    std::map<juce::String, juce::Image> images;
	int imagesDataVersion = 0;

	std::map<int, juce::String> channelToImage;


    std::map<int, std::shared_ptr<MFMParam>> mfmParams;
    std::map<juce::String, std::shared_ptr<MFMControl>> mfmControls;

	void loadImages();
	void loadParams();
	void startNetworkThread();

    void setState(juce::String name, juce::String value);
	juce::String getState(juce::String name);

    std::queue<juce::MidiMessage> internalMidiMessages;

	juce::String lastMidiMessage;

private:
    juce::Synthesiser mySynth;

    juce::dsp::Convolution convolution;
    juce::AudioBuffer<float> dryBuffer; 

    double lastSampleRate;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();


    int currentNoteChannel[128] = { 1 };

	void loadMfmParamsFromFolder(juce::String path);


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhysicsBasedSynthAudioProcessor)
};
