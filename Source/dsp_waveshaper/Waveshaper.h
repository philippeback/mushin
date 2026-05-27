#pragma once

#include <algorithm>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace mushin {

/**
 * S-Curve Waveshaper (Saturation & Hard Clipping)
 */
class Waveshaper {
public:
  Waveshaper() = default;

  void prepare(const juce::dsp::ProcessSpec &spec) {
    sampleRate = spec.sampleRate;
    drive.reset(sampleRate, 0.05); // 50ms ramp
    threshold.reset(sampleRate, 0.05);

    // 100ms decay time constant
    decay = static_cast<float>(std::exp(-1.0 / (sampleRate * 0.1)));
    inputPeak = 0.0f;
  }

  void reset() {
    drive.reset(sampleRate, 0.05);
    threshold.reset(sampleRate, 0.05);
    inputPeak = 0.0f;
  }

  void setDrive(float newDrive) { drive.setTargetValue(newDrive); }
  void setExhaustion(bool isExhausted) { exhausted = isExhausted; }
  void setThreshold(float newThreshold) {
    threshold.setTargetValue(std::abs(newThreshold));
  }
  void setAutoGain(bool enabled) { autogain = enabled; }

  float processSample(int channel, float sample) noexcept {
    juce::ignoreUnused(channel);
    auto currentDrive = drive.getNextValue();
    auto currentThreshold = threshold.getNextValue();

    float x = sample * currentDrive;
    float y = 0.0f;
    if (exhausted) {
      y = std::clamp(x, -currentThreshold, currentThreshold);
    } else {
      y = std::tanh(x / (currentThreshold + 1e-3f)) * (currentThreshold + 1e-3f);
    }

    if (autogain) {
      inputPeak = std::max(std::abs(sample), inputPeak * decay);
      float A = std::clamp(inputPeak, 0.01f, 1.0f);
      float gainComp = A + (1.0f - A) / currentDrive;
      y *= gainComp;
    }
    return y;
  }

  // Keep block process for efficiency if needed elsewhere
  template <typename ProcessContext>
  void process(const ProcessContext &context) noexcept {
    auto &&inputBlock = context.getInputBlock();
    auto &&outputBlock = context.getOutputBlock();

    auto numSamples = inputBlock.getNumSamples();
    auto numChannels = inputBlock.getChannelPointer(0) != nullptr
                           ? inputBlock.getNumChannels()
                           : 0;

    for (size_t sample = 0; sample < numSamples; ++sample) {
      auto currentDrive = drive.getNextValue();
      auto currentThreshold = threshold.getNextValue();

      for (size_t channel = 0; channel < numChannels; ++channel) {
        auto *inputSamples = inputBlock.getChannelPointer(channel);
        auto *outputSamples = outputBlock.getChannelPointer(channel);

        float x = inputSamples[sample] * currentDrive;
        float y = 0.0f;
        if (exhausted) {
          y = std::clamp(x, -currentThreshold, currentThreshold);
        } else {
          y = std::tanh(x / (currentThreshold + 1e-3f)) * (currentThreshold + 1e-3f);
        }

        if (autogain) {
          inputPeak = std::max(std::abs(inputSamples[sample]), inputPeak * decay);
          float A = std::clamp(inputPeak, 0.01f, 1.0f);
          float gainComp = A + (1.0f - A) / currentDrive;
          y *= gainComp;
        }
        outputSamples[sample] = y;
      }
    }
  }

private:
  double sampleRate = 44100.0;
  juce::LinearSmoothedValue<float> drive{1.0f};
  juce::LinearSmoothedValue<float> threshold{1.0f};
  bool exhausted = false;
  bool autogain = false;

  float inputPeak = 0.0f;
  float decay = 0.9997f; // Default for 44.1kHz / 100ms
};

} // namespace mushin
