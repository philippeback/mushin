#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "dsp_waveshaper/Waveshaper.h"
#include "dsp/DualFilterLFOMatrix.h"
#include "dsp_sidechain/SidechainProcessor.h"
#include "dsp/NoiseOscillator.h"
#include "dsp/TranceGateProcessor.h"
#include "dsp/QuantizationErrorProcessor.h"
#include "dsp/DelayProcessor.h"

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

  // Real-time meters
  std::atomic<float> scMeterLevel { 0.0f };
  std::atomic<float> scInputPeak { 0.0f };

  // Bridge Diagnostics
  std::atomic<bool> bridgeWorked { false };
  std::atomic<float> lastUiValue { -1.0f };
  juce::String lastParamId { "none" };

  int getTranceGateCurrentStep() const { return tranceGate.getCurrentStep(); }

private:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  mushin::Waveshaper waveshaper;
  mushin::DualFilterLFOMatrix dualFilterSystem;
  mushin::SidechainProcessor sidechainProcessor;
  mushin::NoiseOscillator noiseOscillator;
  mushin::TranceGateProcessor tranceGate;

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

  // Sidechain parameters
  std::atomic<float>* scActiveParam = nullptr;
  std::atomic<float>* scSourceParam = nullptr;
  std::atomic<float>* scThresholdParam = nullptr;
  std::atomic<float>* scAttackParam = nullptr;
  std::atomic<float>* scReleaseParam = nullptr;
  std::atomic<float>* scModeParam = nullptr;
  std::atomic<float>* scAmountParam = nullptr;
  std::atomic<float>* scTargetParam = nullptr;
  std::atomic<float>* scHpFreqParam = nullptr;
  std::atomic<float>* scLpFreqParam = nullptr;

  // Noise Oscillator parameters
  std::atomic<float>* noiseActiveParam = nullptr;
  std::atomic<float>* noiseTypeParam = nullptr;
  std::atomic<float>* noiseFreqParam = nullptr;
  std::atomic<float>* noiseLevelParam = nullptr;
  std::atomic<float>* noiseRoutingParam = nullptr;
  std::atomic<float>* noiseFmModParam = nullptr;

  // Trance Gate parameters
  std::atomic<float>* tgActiveParam = nullptr;
  std::atomic<float>* tgMixParam = nullptr;
  std::atomic<float>* tgPatternParam = nullptr;
  std::atomic<float>* tgRateParam = nullptr;
  std::atomic<float>* tgStartParam = nullptr;
  std::atomic<float>* tgHoldParam = nullptr;
  std::atomic<float>* tgEndParam = nullptr;
  std::atomic<float>* tgDepthParam = nullptr;

  // Quantization Error
  mushin::QuantizationErrorProcessor quantizationError;
  juce::LinearSmoothedValue<float> smoothedQeDepth;
  juce::LinearSmoothedValue<float> smoothedQeMix;

  std::atomic<float>* qeActiveParam = nullptr;
  std::atomic<float>* qeDepthParam = nullptr;
  std::atomic<float>* qeDownsampleParam = nullptr;
  std::atomic<float>* qeMixParam = nullptr;
  std::atomic<float>* qeLinkParam = nullptr;

  // Delay Feature
  mushin::DelayProcessor delayProcessor;
  juce::LinearSmoothedValue<float> smoothedDelayTime;
  juce::LinearSmoothedValue<float> smoothedDelayFeedback;
  juce::LinearSmoothedValue<float> smoothedDelayMix;

  std::atomic<float>* delayActiveParam = nullptr;
  std::atomic<float>* delayTimeParam = nullptr;
  std::atomic<float>* delayFeedbackParam = nullptr;
  std::atomic<float>* delayMixParam = nullptr;
  std::atomic<float>* delayPingPongParam = nullptr;
  std::atomic<float>* delaySyncParam = nullptr;
  std::atomic<float>* delayTempoParam = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MushinAudioProcessor)
};
