/**
 * @file SynthVoice.h
 * 
 * @brief 
 * 
 * @author
 */


#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"

using namespace juce;


class SynthVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast <SynthSound*>(sound) != nullptr;
    }

	void setValueTree(AudioProcessorValueTreeState& valueTree)
	{
		this->valueTree = &valueTree;
	}

    
    void startNote (int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override
    {
        noteStopped = false;
        timeAfterNoteStop = 0;
		this->pitch = midiNoteNumber;
        this->velocity = velocity;
        frequency = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    }
    
    void stopNote (float velocity, bool allowTailOff) override
    {
        if (allowTailOff) {
            noteStopped = true;
        }
        else {
            clearCurrentNote();
        }
    }
    
    void pitchWheelMoved (int newPitchWheelValue) override
    {
        
    }
    
    void controllerMoved (int controllerNumber, int newControllerValue) override
    {
        
    }
    
    void renderNextBlock (AudioBuffer <float> &outputBuffer, int startSample, int numSamples) override
    {
		const float gain = getParam("gain");
        for (int sample = 0; sample < numSamples; ++sample)
        {
			const float dt = 1.0 / getSampleRate();
			// sawtooth wave
			float currentSample = (float)(2.0 * (time * frequency - floor(0.5 + time * frequency)) * gain * 0.1);

            if (noteStopped) {
                timeAfterNoteStop += dt;
				currentSample *= 1 - timeAfterNoteStop / 0.3;
                if (timeAfterNoteStop > 0.3) {
                    clearCurrentNote();
                    return;
                }
            }

            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
				outputBuffer.addSample(channel, startSample, currentSample);
            }
            ++startSample;
            time += dt;

        }
    }


private:

	AudioProcessorValueTreeState* valueTree;

    float time = 0;
    int pitch;
    double velocity;
    double frequency;

    float timeAfterNoteStop = 0;
    bool noteStopped = false;

	float getParam(String paramId)
	{
		return *valueTree->getRawParameterValue(paramId);
	}
};
