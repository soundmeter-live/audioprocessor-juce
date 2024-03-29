#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : sum_sq(0.0), block_count(0), fifoIndex(0), nextFFTBlockReady(false)
{

    forwardFFT = new juce::dsp::FFT(LEVEL_METER_SIZE);
    backwardFFT = new juce::dsp::FFT(LEVEL_METER_SIZE);
    // Make sure you set the size of the component after
    // you add any child components.
    setSize(1, 1);
    setVisible(false);
    level_type = "A-Weighting";
    avg_block_count = 0;

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }

    // --------------------------------------------------------- //

    // find and select audio driver (called DeviceTypes here)
    juce::AudioIODeviceType* type{};
    const auto& types = deviceManager.getAvailableDeviceTypes();
    for (int i = 0; i < types.size(); i++) {
        // list all device types (until match is found with `DEVICE_TYPE`)
        DBG("device type: " << types.getUnchecked(i)->getTypeName());

        // if this is the correct driver, select it and move on
        const auto& current = types.getUnchecked(i);
        if (current->getTypeName() == DEVICE_TYPE) {
            DBG("found device type");
            type = current;
            break;
        }
    }

    // if driver wasn't found, stop here
    if (type == nullptr) {
        DBG("didn't work");
        jassertfalse;
        return;
    }

    // get all audio input devices available from this driver
    deviceManager.setCurrentAudioDeviceType(DEVICE_TYPE, true);
    const juce::StringArray devices = type->getDeviceNames(true); // true = input devices not outputs
    for (const juce::String& d : devices) {
        DBG("device: " << d);
    }

    // select audio device defined in `DEVICE_NAME`
    juce::AudioDeviceManager::AudioDeviceSetup config = deviceManager.getAudioDeviceSetup();
    config.inputDeviceName = DEVICE_NAME;
    config.useDefaultInputChannels = true;
    juce::String err = deviceManager.setAudioDeviceSetup(config, true);

    // catch errors selecting audio device
    if (err.isNotEmpty()) {
        DBG("error:\n" << err);
        jassertfalse;
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
// This function will be called when the audio device is started, or when
// its settings (i.e. sample rate, block size, etc) are changed.
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    //
}

void MainComponent::pushNextSampleIntoFifo(float sample, int channel_idx)
{
    // if the fifo contains enough data, set a flag to say
    // that the next line should now be rendered..
    if(fifoIndex == LEVEL_METER_SIZE and channel_idx == (AVG_NUM_CHANNELS-1))
    {
        if(!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
        fifo[(size_t) fifoIndex++] = sample/AVG_NUM_CHANNELS;
    }
    else if(fifoIndex == LEVEL_METER_SIZE and channel_idx != (AVG_NUM_CHANNELS-1)){
        fifoIndex = 0;
        fifo[(size_t) fifoIndex++] += (sample/AVG_NUM_CHANNELS);
    }
    else if(channel_idx != 0){
        fifo[(size_t) fifoIndex++] += (sample/AVG_NUM_CHANNELS);
    }
    else{
        fifo[(size_t) fifoIndex++] = (sample/AVG_NUM_CHANNELS);
    }
}

// print and save the metering level
void MainComponent::dBmeter(std::string type) {
    //float p0 = 20e-6;
    if (type == "A-Weighting") {
        aWeighting();
    }
    else if (type == "SPL") {
        // use raw value...
    }
    level = 20.0 * std::log10(calculateRMS() /* / p0 */);
    level += SPL_CALIBRATION;
}

float MainComponent::aWeighting() {
    forwardFFT->performFrequencyOnlyForwardTransform(fftData.data(), true);

    float sum = 0;
    for (int i = 0; i < LEVEL_METER_SIZE; i++) {

        float frequency = deviceManager.getCurrentAudioDevice()->getCurrentSampleRate() / 2 * i / LEVEL_METER_SIZE;
        float f2 = frequency * frequency;
        float level = 1.2588966 * 148840000 * f2 * f2 /
            ((f2 + 424.36) * std::sqrt((f2 + 11599.29) * (f2 + 544496.41)) * (f2 + 148840000));
        //        sum += level * fabsf(fftData[i]);
        fftData[i] *= level;
        sum += fftData[i];
    }
    sum /= LEVEL_METER_SIZE;
    return 20 * std::log(sum) / std::log(10);
}

float MainComponent::calculateRMS() {
    float sumSquares = 0.0;

    for (float sample : fftData) {
        sumSquares += sample * sample;
    }
    float rmsValue = std::sqrt(sumSquares / LEVEL_METER_SIZE);

    return rmsValue;
}

void MainComponent::writeData() {
    float avg_block_level = 0;
    for (int i = 0; i < block_buffer.size(); i++) {
        avg_block_level += block_buffer[i];
    }
    avg_block_level /= block_buffer.size();
    upload_buffer[DataUploader::timeNow()] = avg_block_level;
    DBG("adding new block " << avg_block_level);

    avg_block_count++;
    if (avg_block_count == POINTS_PER_UPLOAD) {
        DBG("\nattempting upload...");
        upLoad();
        avg_block_count = 0;
    }
}

void MainComponent::upLoad() {
    DataUploader upload(upload_buffer);
    upload_buffer.clear();
}

// Your audio-processing code goes here!
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // get block info
    auto* device = deviceManager.getCurrentAudioDevice();
    auto activeInputChannels = device->getActiveInputChannels();
    auto activeOutputChannels = device->getActiveOutputChannels();
    auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
    auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;


    for (auto channel = 0; channel < AVG_NUM_CHANNELS; ++channel)
    {
        // clear any unused buffer data
        if ((!activeOutputChannels[channel]) || maxInputChannels == 0)
        {
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
        }
        else
        {
            auto actualInputChannel = channel % maxInputChannels;
            if (!activeInputChannels[channel])
                bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
            else
            {
                const float* inBuffer = bufferToFill.buffer->getReadPointer(actualInputChannel,
                    bufferToFill.startSample);

                /* SAMPLE PROCESSING LOOP */
                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
                {
                    pushNextSampleIntoFifo(inBuffer[sample], channel);
                    if (nextFFTBlockReady) {
                        // calculate dB values
                        dBmeter(level_type);
                        // add to current block (to be averaged)
                        block_buffer[block_count++] = level;

                        // push completed blocks to block buffer
                        if (block_count == AVG_NUM_BLOCKS) {
                            block_count = 0;
                            writeData();
                        }
                        nextFFTBlockReady = false;
                    }

                }
            }
        }
    }

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    bufferToFill.clearActiveBufferRegion();


}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
