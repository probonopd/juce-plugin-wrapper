#include "WrapperProcessor.h"
#include "WrapperEditor.h"

AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  AudioPluginFormatManager pluginFormatManager;
  pluginFormatManager.addDefaultFormats();

  juce::OwnedArray<juce::PluginDescription> pluginDescriptions;
  juce::KnownPluginList pluginList;

  juce::String pluginPath("C:\\Program Files\\Common Files\\VST3\\Dexed.vst3");

  pluginList.scanAndAddFile(pluginPath, true, pluginDescriptions,
                            *pluginFormatManager.getFormat(0));

  jassert(pluginDescriptions.size() > 0);
  String errorMessage;
  AudioPluginInstance *instance =
      pluginFormatManager
          .createPluginInstance(*pluginDescriptions[0], 44100.0, 512,
                                errorMessage)
          .release();
  if (instance) {
    std::cout << "Loaded plugin instance: " << instance->getName().toStdString()
              << std::endl;
    return new WrapperProcessor(instance);
  }

  DBG("Error instantiating plugin");
  DBG(" ->" + errorMessage);

  return nullptr;
}

//=======================================================================================================================

WrapperProcessor::WrapperProcessor(AudioPluginInstance *processorToUse)
    : AudioProcessor(getBusesPropertiesFromProcessor(processorToUse)),
      plugin(processorToUse) {}

WrapperProcessor::~WrapperProcessor() { releaseResources(); }

AudioProcessor::BusesProperties
WrapperProcessor::getBusesPropertiesFromProcessor(AudioProcessor *processor) {
  BusesProperties retval;

  for (int dir = 0; dir < 2; ++dir) {
    const bool isInput = (dir == 0);
    const int n = processor->getBusCount(isInput);

    for (int i = 0; i < n; ++i)
      if (AudioProcessor::Bus *bus = processor->getBus(isInput, i))
        retval.addBus(isInput, bus->getName(), bus->getDefaultLayout(),
                      bus->isEnabledByDefault());
  }

  return retval;
}

AudioProcessorEditor *WrapperProcessor::createEditor() {
#if 0 // change to 1 to show the selected plug-in's own GUI window, if it has
      // one
    if (plugin->hasEditor()) return plugin->createEditor();
    else
#endif
  return new WrapperEditor(*this);
}

void WrapperProcessor::prepareToPlay(double sampleRate,
                                     int maximumExpectedSamplesPerBlock) {
  plugin->releaseResources();
  plugin->setRateAndBufferSizeDetails(sampleRate,
                                      maximumExpectedSamplesPerBlock);

  // sync number of buses
  for (int dir = 0; dir < 2; ++dir) {
    const bool isInput = (dir == 0);
    int expectedNumBuses = getBusCount(isInput);
    int requiredNumBuses = plugin->getBusCount(isInput);

    for (; expectedNumBuses < requiredNumBuses; expectedNumBuses++)
      plugin->addBus(isInput);

    for (; requiredNumBuses < expectedNumBuses; requiredNumBuses++)
      plugin->removeBus(isInput);
  }

  plugin->setBusesLayout(getBusesLayout());
  plugin->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
}

void WrapperProcessor::releaseResources() { plugin->releaseResources(); }

bool WrapperProcessor::canApplyBusCountChange(
    bool isInput, bool isAddingBuses, BusProperties &outNewBusProperties) {
  if (isAddingBuses) {
    int busIdx = plugin->getBusCount(isInput);

    if (!plugin->addBus(isInput))
      return false;

    if (Bus *bus = plugin->getBus(isInput, busIdx)) {
      outNewBusProperties.busName = bus->getName();
      outNewBusProperties.defaultLayout = bus->getDefaultLayout();
      outNewBusProperties.isActivatedByDefault = bus->isEnabledByDefault();

      return true;
    } else {
      jassertfalse;
      return false;
    }
  } else
    return plugin->removeBus(isInput);
}

void WrapperProcessor::processBlock(AudioBuffer<float> &buffer,
                                    MidiBuffer &midiMessages) {
  // plugin->processBlock(buffer, midiMessages);

  // Create new buffers for the output where we can combine the output of
  // multiple rendering passes
  // TODO: Move this to the header so that the buffers are only created once;
  // but for this we would need to know the number of channels (usually 2) and
  // samples (usually 512)

  AudioBuffer<float> outputBuffer1(buffer.getNumChannels(),
                                   buffer.getNumSamples());
  AudioBuffer<float> outputBuffer2(buffer.getNumChannels(),
                                   buffer.getNumSamples());

  //   std::cout << "Channels: " << buffer.getNumChannels() << std::endl;
  //   std::cout << "Samples: " << buffer.getNumSamples() << std::endl;

  // Initialize the output buffers with zeros
  outputBuffer1.clear();
  outputBuffer2.clear();

  // Set the detune parameter to 0 semitones
  //   plugin->setParameter(3, 0.48f);

  // Use the plugin to render the output
  plugin->processBlock(outputBuffer1, midiMessages);

  // Change the plugin's parameters, set detune to 1 semitone
  //   plugin->setParameter(3, 0.52f);

  //   plugin->processBlock(outputBuffer2, midiMessages);

  // Stereo pan outputBuffer1 to the left
  for (int i = 0; i < outputBuffer1.getNumSamples(); i++) {
    outputBuffer1.setSample(0, i, outputBuffer1.getSample(0, i) * 0.5f);
    outputBuffer1.setSample(1, i, outputBuffer1.getSample(1, i) * 0.0f);
  }

  // Stereo pan outputBuffer2 to the right
  //   for (int i = 0; i < outputBuffer2.getNumSamples(); i++) {
  //     outputBuffer2.setSample(0, i, outputBuffer2.getSample(0, i) * 0.0f);
  //     outputBuffer2.setSample(1, i, outputBuffer2.getSample(1, i) * 0.5f);
  //   }

  // Copy the content of outputBuffer1 to &buffer

  for (int i = 0; i < buffer.getNumChannels(); i++) {
    for (int j = 0; j < buffer.getNumSamples(); j++) {
      buffer.setSample(i, j, outputBuffer1.getSample(i, j));
    }
  }

  // Reset the plugin's parameters
  //   plugin->setParameter(3, 0.5f);

  // Combine the output of the two rendering passes
  // but reduce the volume of each rendering pass by 0.5
  //   for (int i = 0; i < buffer.getNumChannels(); i++) {
  //     for (int j = 0; j < buffer.getNumSamples(); j++) {
  //       buffer.setSample(
  //           i, j,
  //           (outputBuffer1.getSample(i, j) + outputBuffer2.getSample(i, j))
  //           *
  //               0.5f);
  //     }
  //   }
}

size_t WrapperProcessor::getPluginState() {
    return pluginState.getSize();
}

void WrapperProcessor::setPluginState() {}
