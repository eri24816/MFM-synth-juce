/*
  ==============================================================================

    String.h
    Created: 26 Aug 2024 11:18:01pm
    Author:  a931e

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "Util.h"
using namespace juce;

struct SliderSpec {
	String name;
	String paramId;
	float skewFactor = 1;
};

class SliderGroup : public juce::GroupComponent
{

public:
	SliderGroup(PhysicsBasedSynthAudioProcessor& p, String name,std::vector<SliderSpec> sliderSpecs) : juce::GroupComponent(name,name), audioProcessor(p)
	{
		for (SliderSpec spec : sliderSpecs)
		{
			auto slider = new SliderWithName(spec.name, audioProcessor, spec.paramId);
			slider->slider.setSkewFactor(spec.skewFactor);
			addAndMakeVisible(slider);
			
			sliders.push_back(slider);
			sliderMap[spec.paramId] = slider;
		}
	}

	~SliderGroup()
	{
		for (SliderWithName* slider : sliders)
		{
			delete slider;
		}
	}

	void resized() override
	{
		juce::FlexBox fb;
		fb.flexWrap = juce::FlexBox::Wrap::wrap;
		fb.alignItems = juce::FlexBox::AlignItems::stretch;
		fb.justifyContent = juce::FlexBox::JustifyContent::center;
		fb.alignContent = juce::FlexBox::AlignContent::stretch;

		
		for (SliderWithName* slider : sliders)
		{
			fb.items.add(juce::FlexItem(*slider).withMinWidth(70).withMinHeight(90).withMargin(10).withFlex(1));
		}

		fb.performLayout(getLocalBounds());
	}

	SliderWithName* getSlider(String paramId)
	{
		return sliderMap[paramId];
	}

private:
	PhysicsBasedSynthAudioProcessor& audioProcessor;

	std::vector<SliderWithName*> sliders;
	std::map<String, SliderWithName*> sliderMap;

};
