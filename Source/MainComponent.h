#pragma once

#include <JuceHeader.h>

/* CONSTANTS: */

// input device setup: device driver name / input device name
#define DEVICE_TYPE "Windows Audio"
#define DEVICE_NAME "Microphone (Conexant HD Audio)"
// width of command line level meter in chars
#define METER_DISPLAY_SIZE (60)
// choose whether to only show current level or show previous levels as well
#define METER_SHOW_HISTORY (false)


//==============================================================================
/*
	This component lives inside our window, and this is where you should put all
	your controls and content.
*/
class MainComponent : public juce::AudioAppComponent
{
public:
	//==============================================================================
	MainComponent();
	~MainComponent() override;

	//==============================================================================
	void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
	void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
	void releaseResources() override;

	//==============================================================================
	void paint(juce::Graphics& g) override;
	void resized() override;

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
