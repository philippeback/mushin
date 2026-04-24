#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp_waveshaper/Waveshaper.h"
#include "dsp/DualFilterLFOMatrix.h"

class MushinAudioProcessor : public juce::AudioProcessor {
public:
  MushinAudioProcessor();
  ~MushinAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void reset() override;
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

  // Bridge Diagnostics
  std::atomic<bool> bridgeWorked { false };
  std::atomic<float> lastUiValue { -1.0f };
  juce::String lastParamId { "none" };

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  mushin::Waveshaper waveshaper;
  mushin::DualFilterLFOMatrix dualFilterSystem;

  // Pre-allocated dry buffer to avoid allocations in processBlock
  juce::AudioBuffer<float> dryBuffer;

  // Smoothed parameters
  juce::LinearSmoothedValue<float> smoothedGain;
  juce::LinearSmoothedValue<float> smoothedMix;

  // Cached parameter pointers
  std::atomic<float>* gainParam = nullptr;
  std::atomic<float>* driveParam = nullptr;
  std::atomic<float>* exhaustionParam = nullptr;
  std::atomic<float>* thresholdParam = nullptr;
  std::atomic<float>* mixParam = nullptr;

  // New params for Dual Filter
  std::atomic<float>* filterACutoffParam = nullptr;
  std::atomic<float>* filterAResonanceParam = nullptr;
  std::atomic<float>* filterADriveParam = nullptr;
  std::atomic<float>* filterAGritParam = nullptr;
  std::atomic<float>* filterATypeParam = nullptr;
  std::atomic<float>* filterAModeParam = nullptr;

  std::atomic<float>* filterBCutoffParam = nullptr;
  std::atomic<float>* filterBResonanceParam = nullptr;
  std::atomic<float>* filterBDriveParam = nullptr;
  std::atomic<float>* filterBGritParam = nullptr;
  std::atomic<float>* filterBTypeParam = nullptr;
  std::atomic<float>* filterBModeParam = nullptr;

  std::atomic<float>* routingParam = nullptr;

  std::atomic<float>* lfo1FreqParam = nullptr;
  std::atomic<float>* lfo1WaveParam = nullptr;
  std::atomic<float>* lfo2FreqParam = nullptr;
  std::atomic<float>* lfo2WaveParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MushinAudioProcessor)
};
