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
#include "MFMControl.h"



using namespace juce;

namespace {
    float sampleFromArray(float* array, float index) {
        int i = (int)index;
        float a = array[i];
        float b = array[i + 1];
        float interp = index - i;
        return a * (1 - interp) + b * interp;
    }

    class LoopSampler {
	public:
		LoopSampler(float* array, float loopStart, float loopEnd, float overlap = 0) :
            array(array), 
            loopStart(loopStart), 
            loopEnd(loopEnd),
			overlap(overlap)
        {
			loopLength = loopEnd - loopStart;
		}
		float sample(float index, int indexOffset) {
			if (index < loopEnd) {
				return sampleFromArray(array, indexOffset + index);
			}
			index = fmod(index - loopStart, loopLength);
            if (index < overlap) {
				const float lerp = index / overlap;
				return sampleFromArray(array, indexOffset + loopStart + index) * lerp 
                    + sampleFromArray(array, indexOffset + loopEnd + index) * (1 - lerp);
            }
			return sampleFromArray(array, indexOffset + loopStart + index);
		}

	private:
		float* array;
		float loopStart, loopEnd, loopLength, overlap;
	};
}

enum class VoiceState {
	SUSTAIN,
	RELEASE,
	IDLE
};

class SynthVoice : public juce::SynthesiserVoice
{
public:

	SynthVoice() {}

    void prepareToPlay(std::map<int,std::shared_ptr<MFMParam>>& mfmParams, std::map<std::string, std::shared_ptr<MFMControl>>& mfmControls){
        this->mfmParams = mfmParams;
		this->mfmControls = mfmControls;
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

        // select param
		param = mfmParams[midiNoteNumber];

		// initialize loop samplers
		const float loopStart = getParam("loopStart") * param->param_sr;
		const float loopEnd = std::fmin(getParam("loopEnd") * param->param_sr, param->num_samples);
		const float loopOverlap = 0.5 * param->param_sr;
		magGlobal = std::make_unique<LoopSampler>(param->magGlobal.get(), loopStart, loopEnd, loopOverlap);
		alphaGlobal = std::make_unique<LoopSampler>(param->alphaGlobal.get(), loopStart, loopEnd, loopOverlap);

		// select control
		control = mfmControls["test"];

		state = VoiceState::SUSTAIN;
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
			state = VoiceState::RELEASE;
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
		if (state == VoiceState::IDLE) {
			return;
		}
		if (state == VoiceState::RELEASE && timeAfterNoteStop > 0.3) {
			clearCurrentNote();
			state = VoiceState::IDLE;
			return;
		}

		// precompute some constants outside the sample loop
        float recip_two_pi = 1.0f / (2 * float_Pi);
        float twoPi = 2 * float_Pi;
        const float dt = 1.0 / getSampleRate();
        const float attackFactor = 1.0f / param->envelope[(int)(((float)param->attackLen) / param->sampleRate * param->param_sr) - 1];

		// parameters
        const float gain = getParam("gain");
        
        float pIdx = time * param->param_sr;


        for (int sample = 0; sample < numSamples; ++sample)
        {
            time += dt;
			float y = 0;

            
            
            for (int i = 0; i < param->num_partials; i++) {
				int n = i + 1;
				const float mag = magGlobal->sample(pIdx, i * param->num_samples);
				ft[i] += frequency * n * dt;

				// limit 2 pi f t in -pi to pi
				if (ft[i] > 0.5) {
					ft[i] -= 1;
				}
				float alpha = alphaGlobal->sample(pIdx, i * param->num_samples);
                float stuff_in_sin = 2 * float_Pi * ft[i] + alpha;
				stuff_in_sin = stuff_in_sin - twoPi * juce::roundToInt(stuff_in_sin * recip_two_pi);
				y += mag * juce::dsp::FastMathApproximations::sin(stuff_in_sin);
            }
			
			if (state == VoiceState::RELEASE) {
                timeAfterNoteStop += dt;
				y *= exp(-timeAfterNoteStop * 10);
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
	enum VoiceState state = VoiceState::IDLE;

	std::map<int, std::shared_ptr<MFMParam>>& mfmParams = *new std::map<int, std::shared_ptr<MFMParam>>();
    std::shared_ptr<MFMParam> param;
	std::unique_ptr<LoopSampler> magGlobal;
	std::unique_ptr<LoopSampler> alphaGlobal;

	std::map<std::string, std::shared_ptr<MFMControl>>& mfmControls = *new std::map<std::string, std::shared_ptr<MFMControl>>();
	std::shared_ptr<MFMControl> control;



	float getParam(String paramId)
	{
		return *valueTree->getRawParameterValue(paramId);
	}
};
