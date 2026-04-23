#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace mushin {

/**
 * S-Curve Waveshaper (Saturation & Hard Clipping)
 */
class Waveshaper {
public:
    Waveshaper() = default;

    void prepare(const juce::dsp::ProcessSpec& spec) {
        juce::ignoreUnused(spec);
    }

    void reset() {}

    void setDrive(float newDrive) { drive = newDrive; }
    void setExhaustion(bool isExhausted) { exhausted = isExhausted; }
    void setThreshold(float newThreshold) { threshold = std::abs(newThreshold); }

    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept {
        auto&& inputBlock  = context.getInputBlock();
        auto&& outputBlock = context.getOutputBlock();

        auto numSamples  = inputBlock.getNumSamples();
        auto numChannels = inputBlock.getChannelPointer(0) != nullptr ? inputBlock.getNumChannels() : 0;

        for (size_t channel = 0; channel < numChannels; ++channel) {
            auto* inputSamples  = inputBlock.getChannelPointer(channel);
            auto* outputSamples = outputBlock.getChannelPointer(channel);

            for (size_t sample = 0; sample < numSamples; ++sample) {
                outputSamples[sample] = processSample(inputSamples[sample]);
            }
        }
    }

    float processSample(float x) noexcept {
        if (exhausted) {
            return std::clamp(x * drive, -threshold, threshold);
        } else {
            // Soft-clipping: y = tanh(x * drive)
            return std::tanh(x * drive);
        }
    }

private:
    float drive = 1.0f;
    bool exhausted = false;
    float threshold = 1.0f;
};

} // namespace mushin
