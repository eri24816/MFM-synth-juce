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
namespace {
    class NetworkThread : public juce::Thread
    {
	private:
        std::map<juce::String, std::shared_ptr<MFMControl>>* mfmControls;
    public:
		NetworkThread(std::map<juce::String, std::shared_ptr<MFMControl>>* mfmControls)
            : Thread("Network Thread")
			, mfmControls(mfmControls)
        {
        }
        void run() override
        {
            while (!threadShouldExit())
            {
                auto response = URL("http://localhost:9649/serial").readEntireTextStream();
				// trim '"' from response
				response = response.substring(1, response.length() - 1);
				//juce::Logger::writeToLog(response);
				// skip if response is empty
				if (!response.isEmpty()) {
                    // response is a string with format "x1 x2 x3 x4 x5 x6 x7 x8", where x1 to x8 are 0-1023
					int values[8] = { 0 };
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
					control->density[0] = values[1] / 1023.0 - 0.5;
					control->pitch[0] = values[2] / 1023.0;
                    control->hue[0] = values[3] / 1023.0 * 140;
					control->saturation[0] = values[4] / 1023.0;
					control->value[0] = values[5] / 1023.0;
                }
                wait(100);
            }
        }
    };
}

//==============================================================================
/**
*/
class PhysicsBasedSynthAudioProcessor  : public juce::AudioProcessor
{
public: 
    //==============================================================================
    PhysicsBasedSynthAudioProcessor();
    ~PhysicsBasedSynthAudioProcessor() override;

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

private:
    juce::Synthesiser mySynth;

    juce::dsp::Convolution convolution;
    juce::AudioBuffer<float> dryBuffer; 

    double lastSampleRate;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();


    int currentNoteChannel[128] = { 1 };

	void loadMfmParamsFromFolder(juce::String path);

	NetworkThread networkThread;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhysicsBasedSynthAudioProcessor)
};
