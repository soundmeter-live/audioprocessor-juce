#pragma once

#include <JuceHeader.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include "DataUploader.h"

/* CONSTANTS: */

// input device setup: device driver name / input device name
//#define DEVICE_TYPE "Windows Audio"
#define DEVICE_TYPE "CoreAudio"
//#define DEVICE_NAME "Microphone (Conexant HD Audio)"
//#define DEVICE_NAME "Pro Tools Aggregate I/O"
#define DEVICE_NAME "Teensy MIDI_Audio"
// width of command line level meter in chars
#define METER_DISPLAY_SIZE (60)
// choose whether to only show current level or show previous levels as well
#define METER_HIDE_HISTORY (false)

#define LEVEL_METER_SIZE (1024)  // sample block size
#define AVG_NUM_BLOCKS (80)      // number of sample blocks to average per uploaded data point
#define POINTS_PER_UPLOAD (15)   // number of data points to upload together
#define SPL_CALIBRATION (100)    // mic calibration in dBSPL (offset for weighted dbFS values)

#define WRITE_PATH ("/Users/cooper/Documents/senior_design/audioprocessor-juce/data/data.json")
using json = nlohmann::json;


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
    void pushNextSampleIntoFifo(float sample);
    float calculateRMS();

    //==============================================================================
    void writeData();
    void upLoad();

private:

    float sum_sq; // square sum of rms blocks
    int block_count; // iterate over AVG_NUM_BLOCKS
    int avg_block_count;
    int fifoIndex;
    bool nextFFTBlockReady;
    juce::dsp::FFT* forwardFFT;
    juce::dsp::FFT* backwardFFT;
    std::array<float, LEVEL_METER_SIZE> fifo;                    // [4]
    std::array<float, LEVEL_METER_SIZE> fftData;
    float level;
    std::string level_type;
    json data;
    std::array<float, AVG_NUM_BLOCKS> block_buffer;
    std::map<juce::int64, float> upload_buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
