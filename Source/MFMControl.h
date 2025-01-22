/*
  ==============================================================================

    MFMControl.h
    Created: 21 Jan 2025 9:22:05pm
    Author:  a931e

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>

class MFMControl
{
public:
    MFMControl(int length) : length(length) {
        intensity = std::unique_ptr<float[]>(new float[length]);
        pitch = std::unique_ptr<float[]>(new float[length]);
        density = std::unique_ptr<float[]>(new float[length]);
        hue = std::unique_ptr<float[]>(new float[length]);
        saturation = std::unique_ptr<float[]>(new float[length]);
        value = std::unique_ptr<float[]>(new float[length]);
    }
    std::unique_ptr<float[]> intensity, pitch, density, hue, saturation, value;
    int length;
};
namespace {
    std::unique_ptr<float[]> convertFromBase64(const String& base64) {
        MemoryOutputStream stream;
        Base64::convertFromBase64(stream, base64);
        auto size = stream.getDataSize();
        auto data = stream.getData();
        auto result = new float[size];
        memcpy(result, data, size);
        return std::unique_ptr<float[]>(result);

    }

    MFMControl notationToControl(std::string path) {

        MemoryBlock fileData;
        File(path).loadFileAsData(fileData);

        auto postData = new juce::DynamicObject();
        postData->setProperty("image", Base64::toBase64(fileData.getData(), fileData.getSize()));
        auto postStr = JSON::toString(postData);
        auto utf8 = postStr.toRawUTF8();
        auto response = URL("http://localhost:8000/analyze-notation").withPOSTData(utf8).readEntireTextStream();
        auto json = JSON::parse(response);
        int length = json["control_length"];
        MFMControl control(length);
        control.intensity = convertFromBase64(json["intensity"].toString());
        control.pitch = convertFromBase64(json["pitch"].toString());
        control.density = convertFromBase64(json["density"].toString());
        control.hue = convertFromBase64(json["hue"].toString());
        control.saturation = convertFromBase64(json["saturation"].toString());
        control.value = convertFromBase64(json["value"].toString());
        return control;
    }

}