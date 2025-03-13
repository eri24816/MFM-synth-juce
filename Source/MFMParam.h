/*
  ==============================================================================

    ToneParam.h
    Created: 12 Jan 2025 11:01:37pm
    Author:  a931e

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include "cnpy/cnpy.h"



class MFMParam
{
public:
	std::unique_ptr<float[]> magGlobal, attackWave, alphaGlobal, envelope;
    std::unique_ptr<float[]> alphaLocalSpreadingCenter, alphaLocalSpreadingFactor, alphaLocalNoiseGain;
    int param_sr, num_samples, num_partials, overlapLen, attackLen, sampleRate;
	float base_freq, coloredCutoff1, coloredCutoff2;


	MFMParam(std::string path)
    {
		cnpy::NpyArray magGlobalArray = cnpy::read_npz(path, "magRatio");
        num_samples = magGlobalArray.shape[1];
        num_partials = magGlobalArray.shape[0];


        param_sr = cnpy::read_npz(path, "par_sr").as_vec<int>()[0];
		attackLen = cnpy::read_npz(path, "attackLen").as_vec<int>()[0];
        overlapLen = cnpy::read_npz(path, "overlapLen").as_vec<int>()[0];
        overlapLen = attackLen / 2;
		sampleRate = cnpy::read_npz(path, "sampleRate").as_vec<int>()[0];
		magGlobal = load_np_into_array(path, "magRatio");
		attackWave = load_np_into_array(path, "attackWave");
		alphaGlobal = load_np_into_array(path, "alphaGlobal");
		envelope = load_np_into_array(path, "totalEnv");
		base_freq = cnpy::read_npz(path, "pitch").as_vec<float>()[0];

		alphaLocalSpreadingCenter = load_np_into_array(path, "alphaLocal.spreadingCenter");
		alphaLocalSpreadingFactor = load_np_into_array(path, "alphaLocal.spreadingFactor");
		alphaLocalNoiseGain = load_np_into_array(path, "alphaLocal.noiseGain");
       
		coloredCutoff1 = cnpy::read_npz(path, "coloredCutoff1").as_vec<float>()[0];
		coloredCutoff2 = cnpy::read_npz(path, "coloredCutoff2").as_vec<float>()[0];
    }

    std::unique_ptr<float[]> load_np_into_array(std::string path, std::string key) {
		auto npy_array = cnpy::read_npz(path, key);
        auto npy_data = npy_array.data<float>();
        auto array = new float[npy_array.num_vals];
        for (int i = 0; i < npy_array.num_vals; i++) {
            array[i] = npy_data[i];
        }
		return std::unique_ptr<float[]>(array);
    }


private:
    
};