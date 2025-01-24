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

	class TailSampler {
	public:
		TailSampler(float* array, int length, float rightMargin) :
			array(array),
			length(length),
			rightMargin(rightMargin)
		{
			jassert(rightMargin < length);
		}
		float sample(float index, int indexOffset) {
			index = std::fmin(length-1-rightMargin, index);
			return sampleFromArray(array, indexOffset + index);
		}

	private:
		float* array;
		int length;
		float rightMargin;
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

    void prepareToPlay(
        std::map<int, std::shared_ptr<MFMParam>>* mfmParams,
        std::map<juce::String, std::shared_ptr<MFMControl>>* mfmControls,
		std::map<int, juce::String>* channelToImage,
        int currentNoteChannel[128]
    ){
        this->mfmParams = mfmParams;
		this->mfmControls = mfmControls;
		this->currentNoteChannel = currentNoteChannel;
		this->channelToImage = channelToImage;
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
        if (mfmParams->find(midiNoteNumber) == mfmParams->end()) {
			param.reset();
			clearCurrentNote();
            state = VoiceState::IDLE;
			return;
        }

        // select param
		param = (*mfmParams)[midiNoteNumber];

		// initialize loop samplers
		const float loopStart = getParam("loopStart") * param->param_sr;
		const float loopEnd = std::fmin(getParam("loopEnd") * param->param_sr, param->num_samples);
		const float loopOverlap = 0.5 * param->param_sr;
		magGlobal = std::make_unique<LoopSampler>(param->magGlobal.get(), loopStart, loopEnd, loopOverlap);
		alphaGlobal = std::make_unique<LoopSampler>(param->alphaGlobal.get(), loopStart, loopEnd, loopOverlap);

		// select control
		/*juce::String controlToUse = (*channelToImage)[currentNoteChannel[midiNoteNumber]];
		if (controlToUse == nullptr) {
			state = VoiceState::IDLE;
			return;
		}*/

		auto currentChannel = currentNoteChannel[midiNoteNumber];
		if (channelToImage->find(currentChannel) == channelToImage->end()) {
			state = VoiceState::IDLE;
			return;
		}
		auto controlName = (*channelToImage)[currentChannel];
		if (mfmControls->find(controlName) == mfmControls->end()) {
			state = VoiceState::IDLE;
			return;
		}
		auto control = (*mfmControls)[controlName];

		intensityS = std::make_unique<TailSampler>(control->intensity.get(), control->length, 5);
		pitchS = std::make_unique<TailSampler>(control->pitch.get(), control->length, 5);
		densityS = std::make_unique<TailSampler>(control->density.get(), control->length, 5);
		hueS = std::make_unique<TailSampler>(control->hue.get(), control->length, 5);
		saturationS = std::make_unique<TailSampler>(control->saturation.get(), control->length, 5);
		valueS = std::make_unique<TailSampler>(control->value.get(), control->length, 5);


		state = VoiceState::SUSTAIN;
        timeAfterNoteStop = 0;
        time = 0;
		for (int i = 0; i < param->num_partials; i++) {
            ft[i] = 0;
		}
		this->pitch = midiNoteNumber;
        this->velocity = velocity;
        baseFrequency = MidiMessage::getMidiNoteInHertz(midiNoteNumber);
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

        const float gain = getParam("gain");
        
        float pIdx = time * param->param_sr;
		
		// control
		
		float timbreGain = 1.4;

		float cIdx = time * 50.0f; // 50 Hz control rate
		float intensity = intensityS->sample(cIdx, 0);
		float pitch = pitchS->sample(cIdx, 0);
		float density = densityS->sample(cIdx, 0);
		float hue = hueS->sample(cIdx, 0);
		float saturation = saturationS->sample(cIdx, 0);
		float value = valueS->sample(cIdx, 0);

		// translate controls to control vector
		hue = std::fmin(hue, 135);
		hue = std::max(hue, 0.0f);
		float bowPos = 1 / (hue / 135 * 5 + 2);

		const float frequency = baseFrequency * exp2f(pitch / 12); // pitch in semitones

		float magControl[100] = { 1 };
		float alphaControl[100] = { 1 };

		for (int i = 0; i < param->num_partials; i++) {
			int n = i + 1;
			float overtoneFreq = frequency * n;
			// apply intensity
			magControl[i] = intensity;
			// apply bowPos
			magControl[i] *= (1 - (1 - std::fmin(std::abs(n - (1.0f / bowPos)), 1)) * (timbreGain - 1));

			// apply saturation
			if (overtoneFreq <= 1000) {
				magControl[i] *= pow(10, (-4 + saturation * 6) / 20);
			}

			// apply value
			if (overtoneFreq >= 5000) {
				magControl[i] *= pow(10, (-4 + value * 6) / 20);
			}

			alphaControl[i] = 1;
			//TODO: apply density to noise
		}


        for (int sample = 0; sample < numSamples; ++sample)
        {
            time += dt;
			float y = 0;

            for (int i = 0; i < param->num_partials; i++) {
				int n = i + 1;
				const float mag = magGlobal->sample(pIdx, i * param->num_samples) * magControl[i];
				ft[i] += frequency * n * dt;

				// limit 2 pi f t in -pi to pi
				if (ft[i] > 0.5) {
					ft[i] -= 1;
				}
				float alpha = alphaGlobal->sample(pIdx, i * param->num_samples) * alphaControl[i];
                float stuff_in_sin = 2 * float_Pi * ft[i] + alpha;
				stuff_in_sin = stuff_in_sin - twoPi * juce::roundToInt(stuff_in_sin * recip_two_pi);
				y += mag * juce::dsp::FastMathApproximations::sin(stuff_in_sin);
            }
			
			if (state == VoiceState::RELEASE) {
                timeAfterNoteStop += dt;
				y *= exp(-timeAfterNoteStop * 10);
            }

			// If we are in the attack phase, apply the attack
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
    double baseFrequency;

    float timeAfterNoteStop;
	enum VoiceState state = VoiceState::IDLE;

	std::map<int, std::shared_ptr<MFMParam>>* mfmParams = nullptr;
    std::shared_ptr<MFMParam> param;
	std::unique_ptr<LoopSampler> magGlobal;
	std::unique_ptr<LoopSampler> alphaGlobal;

	std::map<juce::String, std::shared_ptr<MFMControl>>* mfmControls = nullptr;
	std::shared_ptr<MFMControl> control;

	std::map<int, juce::String>* channelToImage = nullptr;

	std::unique_ptr<TailSampler> intensityS, pitchS, densityS, hueS, saturationS, valueS;

	int* currentNoteChannel;

	float getParam(String paramId)
	{
		return *valueTree->getRawParameterValue(paramId);
	}
};
