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
#include "cnpy/cnpy.h"
#include <memory>
#include <vector>
#include "MFMParam.h"



using namespace juce;


class SynthVoice : public juce::SynthesiserVoice
{
public:

	SynthVoice() {}

    void prepareToPlay(std::map<int,std::shared_ptr<MFMParam>>& mfmParams) {
        this->mfmParams = mfmParams;
    }

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
		// if mfmParams do not have midiNoteNumber, play nothing
        if (mfmParams.find(midiNoteNumber) == mfmParams.end()) {
			param.reset();
			clearCurrentNote();
			return;
        }

		param = mfmParams[midiNoteNumber];
        noteStopped = false;
        timeAfterNoteStop = 0;
        time = 0;
		for (int i = 0; i < param->num_partials; i++) {
            ft[i] = 0;
		}
		this->pitch = midiNoteNumber;
        this->velocity = velocity;
        frequency = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        //frequency = param->base_freq;
        
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
		if(param == nullptr) {
	        return;
        }


        const float dt = 1.0 / getSampleRate();
		const float gain = getParam("gain");
        float u_float = time * param->param_sr;
		int u = juce::roundToInt(u_float-0.5);
		float interp_a = (u_float - u);
		float interp_b = 1 - interp_a;

		float recip_two_pi = 1.0f / (2 * float_Pi);
		float two_pi = 2 * float_Pi;

		const float attackFactor = 1.0f / param->envelope[(int)(((float)param->attackLen)/param->sampleRate*param->param_sr) - 1];


        for (int sample = 0; sample < numSamples; ++sample)
        {
            time += dt;
			float y = 0;
            
			if (u + 1 < param->num_samples)
			{
                for (int i = 0; i < param->num_partials; i++) {
				    int n = i + 1;
				    const float mag = param->magGlobal[i * param->num_samples + u] * interp_a + param->magGlobal[i * param->num_samples + u + 1] * interp_b;
					if (mag > 10 || mag < -10) {
						throw std::runtime_error("mag is too large");
					}
					ft[i] += frequency * n * dt;

					// limit 2pift in -pi to pi
					if (ft[i] > 0.5) {
						ft[i] -= 1;
					}
					float alpha = param->alphaGlobal[i * param->num_samples + u] * interp_a + param->alphaGlobal[i * param->num_samples + u + 1] * interp_b;
					float stuff_in_sin = 2 * float_Pi * ft[i] + alpha;
					stuff_in_sin = stuff_in_sin - two_pi * juce::roundToInt(stuff_in_sin * recip_two_pi);
					y += mag * juce::dsp::FastMathApproximations::sin(stuff_in_sin);
                }
			}
            if (noteStopped) {
                timeAfterNoteStop += dt;
				y *= 1 - timeAfterNoteStop / 0.3;
                if (timeAfterNoteStop > 0.3) {
                    clearCurrentNote();
                    return;
                }
            }


            int attackU = juce::roundToInt(time * param->sampleRate);
            if (attackU < param->attackLen) {
                int startOverlap = param->attackLen - param->overlapLen;
                if (attackU > startOverlap) {
					float lerp = (attackU-(float)startOverlap) / param->overlapLen;
					float lerp1 = sqrtf(lerp);
					float lerp2 = sqrtf(1-lerp);
					// cosine crossfade
					//float lerp1 = sinf(lerp * float_Pi * 0.5);
					//float lerp2 = sinf((1 - lerp) * float_Pi * 0.5);
					y = param->attackWave[attackU]*getParam("attack")*attackFactor * lerp2 + y * lerp1;
                }
                else {
                    y = param->attackWave[attackU] * getParam("attack") * attackFactor;
                }
            }


            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
				outputBuffer.addSample(channel, startSample, y * gain * 0.1);
            }
            ++startSample;
        }
    }


private:

	AudioProcessorValueTreeState* valueTree;

    float time = 0;
	std::vector<float> ft = std::vector<float>(100);
    int pitch;
    double velocity;
    double frequency;

    float timeAfterNoteStop;
    bool noteStopped = false;
	std::map<int, std::shared_ptr<MFMParam>>& mfmParams = *new std::map<int, std::shared_ptr<MFMParam>>();
    std::shared_ptr<MFMParam> param;


	float getParam(String paramId)
	{
		return *valueTree->getRawParameterValue(paramId);
	}
};
