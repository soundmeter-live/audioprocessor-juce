#pragma once

#include <JuceHeader.h>

/* CONSTANTS: */

// input device setup: device driver name / input device name
//#define DEVICE_TYPE "Windows Audio"
#define DEVICE_TYPE "CoreAudio"
//#define DEVICE_NAME "Microphone (Conexant HD Audio)"
#define DEVICE_NAME "Pro Tools Aggregate I/O"
// width of command line level meter in chars
#define METER_DISPLAY_SIZE (60)
// choose whether to only show current level or show previous levels as well
#define METER_HIDE_HISTORY (false)

#define AVG_NUM_BLOCKS (8)

#define LEVEL_METER_SIZE (1024)


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
    
    //==============================================================================
    void dBmeter(std::string type);
    float aWeighting();
    void pushNextSampleIntoFifo (float sample);
    float calculateRMS();

private:

	float sum_sq; // square sum of rms blocks
	int block_count; // iterate over AVG_NUM_BLOCKS
    int fifoIndex;
    bool nextFFTBlockReady;
    juce::dsp::FFT * forwardFFT;
    juce::dsp::FFT * backwardFFT;
    std::array<float, LEVEL_METER_SIZE> fifo;                    // [4]
    std::array<float, LEVEL_METER_SIZE> fftData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
