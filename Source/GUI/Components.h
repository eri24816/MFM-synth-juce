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