/*
  ==============================================================================

    Util.h
    Created: 27 Aug 2024 12:51:59am
    Author:  a931e

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include <memory>
#include <cmath>

using namespace juce;

class SliderWithName : public Component {
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;


public:
    Slider slider;
    Label nameLabel;
	SliderWithName(String varName, PhysicsBasedSynthAudioProcessor& p, String paramId) :nameLabel("nameLabel", varName), audioProcessor(p) {
		
		setSize(300, 600);

		attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.valueTree, paramId , slider);

        auto param = audioProcessor.valueTree.getParameter(paramId);
		RangedAudioParameter* rangedParam = dynamic_cast<RangedAudioParameter*>(param);
		auto temp = rangedParam->getNormalisableRange();
        slider.setRange(rangedParam->getNormalisableRange().start, rangedParam->getNormalisableRange().end,0);

        slider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);

        slider.setTextBoxStyle(juce::Slider::TextBoxBelow,
            false,
            slider.getTextBoxWidth(),
            slider.getTextBoxHeight());
			

        //setRotaryParameters(-2.5 + 6.283, 2.5 + 6.283, true);
        //setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);

        nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(slider);
        addAndMakeVisible(nameLabel);
    };
    void resized() {
		slider.setBounds(getLocalBounds().getProportion(juce::Rectangle<float>(0, 0, 1, .8)));
        nameLabel.setBounds(getLocalBounds().getProportion(juce::Rectangle<float>(0, .8, 1, .2)));
    }

private:
    PhysicsBasedSynthAudioProcessor& audioProcessor;
};