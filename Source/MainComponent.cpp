#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
	// Make sure you set the size of the component after
	// you add any child components.
	setSize(800, 600);

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
	const juce::String DEVICE_TYPE = "Windows Audio";
	const juce::String DEVICE_NAME = "CABLE Output (VB-Audio Virtual Cable)";

	juce::AudioIODeviceType* type{};

	const auto& types = deviceManager.getAvailableDeviceTypes();
	for (int i = 0; i < types.size(); i++) {
		DBG("device type: "<< types.getUnchecked(i)->getTypeName());

		const auto& current = types.getUnchecked(i);

		if (current->getTypeName() == DEVICE_TYPE) {
			DBG("found device type");
			type = current;
			break;
		}
	}

	if (type == nullptr) {
		DBG("didn't work");
		jassertfalse;
		return;
	}

	//deviceManager.setCurrentAudioDeviceType(types.getUnchecked(DEVICE_TYPE)->getTypeName(), true);
	deviceManager.setCurrentAudioDeviceType(DEVICE_TYPE, true);

	const juce::StringArray devices = type->getDeviceNames(true); // true = input devices not ouputs

	for (int i = 0; i < devices.size(); i++) {
		DBG("device: " << devices[i]);
	}

	juce::AudioDeviceManager::AudioDeviceSetup config = deviceManager.getAudioDeviceSetup();

	config.inputDeviceName = DEVICE_NAME;
	config.useDefaultInputChannels = true;

	juce::String err = deviceManager.setAudioDeviceSetup(config, true);

	if (err.isNotEmpty()) {
		DBG("error:\n" << err);
		jassertfalse;
	}

	counter = 0.0;

}

MainComponent::~MainComponent()
{
	// This shuts down the audio device and clears the audio source.
	shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
	// This function will be called when the audio device is started, or when
	// its settings (i.e. sample rate, block size, etc) are changed.

	// You can use this function to initialise any resources you might need,
	// but be careful - it will be called on the audio thread, not the GUI thread.

	// For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
	// Your audio-processing code goes here!

	// For more details, see the help for AudioProcessor::getNextAudioBlock()

	// Right now we are not producing any data, in which case we need to clear the buffer
	// (to prevent the output of random noise)
	/*bufferToFill.clearActiveBufferRegion();*/

	auto* device = deviceManager.getCurrentAudioDevice();
	auto activeInputChannels = device->getActiveInputChannels();
	auto activeOutputChannels = device->getActiveOutputChannels();
	auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
	auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

	//auto level = (float)levelSlider.getValue();

	for (auto channel = 0; channel < maxOutputChannels; ++channel)
	{
		if ((!activeOutputChannels[channel]) || maxInputChannels == 0)
		{
			bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
		}
		else
		{
			auto actualInputChannel = channel % maxInputChannels; // [1]

			if (!activeInputChannels[channel]) // [2]
			{
				bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
			}
			else // [3]
			{
				const float* inBuffer = bufferToFill.buffer->getReadPointer(actualInputChannel,
					bufferToFill.startSample);

				for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
				{
					counter += fabsf(inBuffer[sample]);
				}
			}
		}
	}

	DBG(counter);

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
