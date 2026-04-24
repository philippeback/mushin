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
  }

  void reset() {
    drive.reset(sampleRate, 0.05);
    threshold.reset(sampleRate, 0.05);
  }

  void setDrive(float newDrive) { drive.setTargetValue(newDrive); }
  void setExhaustion(bool isExhausted) { exhausted = isExhausted; }
  void setThreshold(float newThreshold) {
    threshold.setTargetValue(std::abs(newThreshold));
  }

  float processSample(int channel, float sample) noexcept {
    juce::ignoreUnused(channel);
    auto currentDrive = drive.getNextValue();
    auto currentThreshold = threshold.getNextValue();

    float x = sample * currentDrive;
    if (exhausted) {
      return std::clamp(x, -currentThreshold, currentThreshold);
    } else {
      return std::tanh(x / (currentThreshold + 1e-3f)) * (currentThreshold + 1e-3f);
    }
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

        if (exhausted) {
          outputSamples[sample] =
              std::clamp(inputSamples[sample] * currentDrive, -currentThreshold,
                         currentThreshold);
        } else {
          outputSamples[sample] =
              std::tanh((inputSamples[sample] * currentDrive) /
                        (currentThreshold + 1e-3f)) *
              (currentThreshold + 1e-3f);
        }
      }
    }
  }

private:
  double sampleRate = 44100.0;
  juce::LinearSmoothedValue<float> drive{1.0f};
  juce::LinearSmoothedValue<float> threshold{1.0f};
  bool exhausted = false;
};

} // namespace mushin
