#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <algorithm>

namespace mushin {

/**
 * S-Curve Waveshaper (Saturation & Hard Clipping)
 * 
 * Implement a soft-clipping waveshaper using a hyperbolic tangent function: y = tanh(x * drive).
 * When exhaustion triggers, it switches to a hard-clip threshold.
 */
class Waveshaper {
public:
    Waveshaper() = default;

    /**
     * Prepare the processor for playback.
     */
    void prepare(const juce::dsp::ProcessSpec& spec) {
        juce::ignoreUnused(spec);
    }

    /**
     * Reset any internal state.
     */
    void reset() {}

    /**
     * Set the drive amount for soft clipping.
     * Higher drive increases saturation.
     */
    void setDrive(float newDrive) {
        drive = newDrive;
    }

    /**
     * Set the exhaustion state.
     * When true, switches from soft tanh clipping to hard clipping.
     */
    void setExhaustion(bool isExhausted) {
        exhausted = isExhausted;
    }

    /**
     * Set the hard clipping threshold.
     */
    void setThreshold(float newThreshold) {
        threshold = std::abs(newThreshold);
    }

    /**
     * Process a block of audio.
     */
    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept {
        auto&& inputBlock  = context.getInputBlock();
        auto&& outputBlock = context.getOutputBlock();

        auto numSamples  = inputBlock.getNumSamples();
        auto numChannels = inputBlock.getNumChannels();

        for (size_t channel = 0; channel < numChannels; ++channel) {
            auto* inputSamples  = inputBlock.getChannelPointer(channel);
            auto* outputSamples = outputBlock.getChannelPointer(channel);

            for (size_t sample = 0; sample < numSamples; ++sample) {
                outputSamples[sample] = processSample(inputSamples[sample]);
            }
        }
    }

    /**
     * Process a single sample.
     */
    float processSample(float x) noexcept {
        if (exhausted) {
            // Hard-clip threshold: "physically squares off the audio waveform"
            return std::clamp(x, -threshold, threshold);
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
