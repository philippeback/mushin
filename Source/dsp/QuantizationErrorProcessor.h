#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <algorithm>

namespace mushin {

class QuantizationErrorProcessor
{
public:
    QuantizationErrorProcessor() = default;
    ~QuantizationErrorProcessor() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            heldSample[ch] = 0.0f;
            sampleCounter[ch] = 0;
        }
    }

    /**
     * Processes a single audio sample.
     * @param channel The channel index (0 for Left, 1 for Right).
     * @param sample The input floating point sample.
     * @param bitDepth The target resolution bit depth (continuous, 2.0 to 24.0).
     * @param downsample The downsample factor (1 to 32).
     * @param mix The dry/wet mix of this effect (0.0 to 1.0).
     */
    float processSample(int channel, float sample, float bitDepth, int downsample, float mix) noexcept
    {
        if (mix <= 0.001f)
            return sample;

        float wetSample = sample;

        // --- 1. Time-Domain Resolution Decay (Downsampling) ---
        if (downsample > 1)
        {
            if (sampleCounter[channel] % downsample == 0)
            {
                heldSample[channel] = wetSample;
            }
            wetSample = heldSample[channel];
            sampleCounter[channel]++;
        }
        else
        {
            sampleCounter[channel] = 0;
        }

        // --- 2. Amplitude-Domain Resolution Decay (Quantization) ---
        if (bitDepth < 15.5f)
        {
            // Calculate step scale based on the continuous bit depth
            // We use std::max to enforce a safe minimum of 2.0 bits (prevents division by zero)
            float safeDepth = std::max(2.0f, bitDepth);
            float steps = std::pow(2.0f, safeDepth - 1.0f);

            // Bipolar mid-tread quantization
            wetSample = std::round(wetSample * steps) / steps;
        }

        // --- 3. Dry/Wet Crossfade ---
        return (1.0f - mix) * sample + mix * wetSample;
    }

private:
    double currentSampleRate = 44100.0;
    float heldSample[2] = { 0.0f, 0.0f };
    uint32_t sampleCounter[2] = { 0, 0 };
};

} // namespace mushin
