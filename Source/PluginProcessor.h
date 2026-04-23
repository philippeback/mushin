#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp_waveshaper/Waveshaper.h"

class MushinAudioProcessor : public juce::AudioProcessor {
public:
  MushinAudioProcessor();
  ~MushinAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;
  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState treeState;

  // FIFO for the UI to read
  static constexpr int fifoSize = 8192;
  juce::AbstractFifo abstractFifo{fifoSize};
  std::array<float, fifoSize> audioFifo;

  void pushNextSampleIntoFifo(float sample) noexcept;

  std::atomic<bool> bridgeWorked { false };
  std::atomic<float> lastUiValue { -1.0f };
  juce::String lastParamId { "none" };

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  mushin::Waveshaper waveshaper;

  // Cached parameter pointers
  std::atomic<float>* gainParam = nullptr;
  std::atomic<float>* driveParam = nullptr;
  std::atomic<float>* exhaustionParam = nullptr;
  std::atomic<float>* thresholdParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MushinAudioProcessor)
};
