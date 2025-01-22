/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PhysicsBasedSynthAudioProcessorEditor::PhysicsBasedSynthAudioProcessorEditor (PhysicsBasedSynthAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), 
	mainParamComponent(p, "Main", {
		{"Gain", "gain"},
		{"Wet Dry", "wetDry"},
		{"Attack", "attack"},
		{"Loop Start", "loopStart"},
		{"Loop End", "loopEnd"}
		})
{
	setSize(1000, 700);

	addAndMakeVisible(mainParamComponent);
}

PhysicsBasedSynthAudioProcessorEditor::~PhysicsBasedSynthAudioProcessorEditor()
{
}

//==============================================================================
void PhysicsBasedSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void PhysicsBasedSynthAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    Rectangle<int> area = getLocalBounds();

	juce::FlexBox fb;
	fb.flexDirection = FlexBox::Direction::row;

	fb.items.add(FlexItem(mainParamComponent).withFlex(1).withMargin(5));
	fb.performLayout(area);
}
