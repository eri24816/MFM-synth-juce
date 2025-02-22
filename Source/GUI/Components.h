/*
  ==============================================================================

    Components.h
    Created: 23 Jan 2025 7:49:45pm
    Author:  a931e

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

/*
display a list of images with their names at their top-left corner
image1 name1
image2 name2
image3 name3

*/
class ImageShowcase : public juce::Component, juce::Timer
{
public:
	ImageShowcase(PhysicsBasedSynthAudioProcessor& p)
		: p(p)
	{
		startTimerHz(10);
		setSize(100, 100);
	}
	void paint(juce::Graphics& g) override
	{

		lastImagesDataVersion = p.imagesDataVersion;
		int x = 0;
		int y = 0;
		int width = 100;
		int height = 100;
		for (auto& image : p.images)
		{
			g.drawImageWithin(image.second, x, y, image.second.getWidth() * imgScale, 80, juce::RectanglePlacement::stretchToFit);
			g.drawFittedText(image.first, x, y, image.second.getWidth() * imgScale, 20, juce::Justification::topLeft, 1);
			y += 80 + 10;
			width = std::fmax(width, image.second.getWidth() * imgScale);
			height += 80+10;
		}
		setSize(width, height);
	}
	void timerCallback() override
	{
		if (lastImagesDataVersion != p.imagesDataVersion)
		{
			repaint();
		}
	}
private:
	PhysicsBasedSynthAudioProcessor& p;
	int lastImagesDataVersion = 0;
	float imgScale = 0.2;
};

/*
* Use flexbox to layout the components
* 
text:
"Set these settings and apply them before using the plugin"

input boxes:
server address and port: string
images directory: string
table directory: string

buttons:
apply settings

*/

class InputBoxWithLabel : public juce::Component
{
public:
	InputBoxWithLabel(juce::String label, juce::String propertyName, juce::ValueTree& valueTree)
		: label(label), propertyName(propertyName), valueTree(valueTree)
	{
		addAndMakeVisible(text);
		addAndMakeVisible(inputBox);
		text.setText(label, juce::dontSendNotification);
		inputBox.setText(valueTree.getProperty(propertyName, juce::String()), juce::dontSendNotification);
		inputBox.onTextChange = [this]() {
			this->valueTree.setProperty(this->propertyName, inputBox.getText(), nullptr);
			};
	}
	void paint(juce::Graphics& g) override
	{
	}
	void resized() override
	{
		juce::FlexBox fb;
		fb.flexDirection = FlexBox::Direction::column;
		fb.items.add(FlexItem(text).withFlex(1).withMargin(5));
		fb.items.add(FlexItem(inputBox).withFlex(1.5).withMargin(5));
		fb.justifyContent = FlexBox::JustifyContent::flexStart;
		fb.performLayout(getLocalBounds());
	}
	juce::TextEditor inputBox;
private:
	juce::String label;
	juce::String propertyName;
	juce::ValueTree& valueTree;
	juce::Label text;
};

#define MFM_VERSION "8"
class SettingsComponent : public juce::Component, juce::Timer
{
public:
	SettingsComponent(PhysicsBasedSynthAudioProcessor& p)
		: p(p)
	{
		startTimerHz(10);

		/*addAndMakeVisible(serverAddress);
		addAndMakeVisible(imagesDirectory);*/
		addAndMakeVisible(tableDirectory);
		addAndMakeVisible(applySettingsButton);
		addAndMakeVisible(statusText);
		addAndMakeVisible(lastMidiMessageText);
		addAndMakeVisible(versionText);
		versionText.setText("MFM Synth Version: " MFM_VERSION, juce::dontSendNotification);
		applySettingsButton.onClick = [this]() {

			// disable the boxes and button
			this->serverAddress.inputBox.setEnabled(false);
			this->imagesDirectory.inputBox.setEnabled(false);
			this->tableDirectory.inputBox.setEnabled(false);
			this->applySettingsButton.setEnabled(false);

			this->p.setState("ServerUrl", this->serverAddress.inputBox.getText());
			this->p.setState("ImagesDirectory", this->imagesDirectory.inputBox.getText());
			this->p.setState("TableDirectory", this->tableDirectory.inputBox.getText());
			auto test = this->p.getState("TableDirectory");
			if (!this->p.getState("ImagesDirectory").isEmpty()) {
				try {
					this->p.loadImages();
				}
				catch (std::exception e) {
					statusText.setText("Error loading images. Please check the directory.", juce::dontSendNotification);
					return;
				}
			}
			try{
				this->p.loadParams();
			}
			catch (std::exception e) {
				statusText.setText("Error loading params.", juce::dontSendNotification);
				return;
			}
			try {
				this->p.startNetworkThread();
			}
			catch (std::exception e) {
				statusText.setText("Error connecting to server.", juce::dontSendNotification);
				return;
			}
			statusText.setText("Done.", juce::dontSendNotification);
		};
		statusText.setText("Apply settings before using the synth.", juce::dontSendNotification);

	}
	void paint(juce::Graphics& g) override
	{
	}
	void resized() override
	{
		juce::FlexBox fb;
		fb.flexDirection = FlexBox::Direction::column;
		/*fb.items.add(FlexItem(serverAddress).withFlex(1).withMargin(5));
		fb.items.add(FlexItem(imagesDirectory).withFlex(1).withMargin(5));*/
		fb.items.add(FlexItem(tableDirectory).withFlex(1).withMargin(5));
		fb.items.add(FlexItem(applySettingsButton).withFlex(1).withMargin(5));
		fb.items.add(FlexItem(statusText).withFlex(1).withMargin(2));
		fb.items.add(FlexItem(lastMidiMessageText).withFlex(1).withMargin(2));
		fb.items.add(FlexItem(versionText).withFlex(1).withMargin(5));
		fb.performLayout(getLocalBounds().withHeight(400));
	}
	void timerCallback() override
	{
		if (p.lastMidiMessage != lastMidiMessage) {
			lastMidiMessage = p.lastMidiMessage;
			lastMidiMessageText.setText(lastMidiMessage, juce::dontSendNotification);
		}
	}
private:
	PhysicsBasedSynthAudioProcessor& p;
	InputBoxWithLabel serverAddress = InputBoxWithLabel("ServerUrl", "ServerUrl", p.valueTree.state);
	InputBoxWithLabel imagesDirectory = InputBoxWithLabel("ImagesDirectory", "ImagesDirectory", p.valueTree.state);
	InputBoxWithLabel tableDirectory = InputBoxWithLabel("TableDirectory", "TableDirectory", p.valueTree.state);
	juce::TextButton applySettingsButton = juce::TextButton("ApplySettings");
	//status text
	juce::Label statusText;
	juce::Label lastMidiMessageText;
	juce::String lastMidiMessage;
	juce::Label versionText;
};
