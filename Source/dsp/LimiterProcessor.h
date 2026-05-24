#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <algorithm>
#include <cmath>

namespace mushin {

class LimiterProcessor
{
public:
    LimiterProcessor() = default;
    ~LimiterProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        numChannels = spec.numChannels;

        // Allocate lookahead buffers (2.0ms max lookahead at 96kHz is 192 samples)
        // Allocate up to 256 samples to be completely safe
        int lookaheadSamples = std::clamp(static_cast<int>((2.0f / 1000.0f) * sampleRate), 1, 256);
        
        lookaheadBuffer.assign (numChannels, std::vector<float> (lookaheadSamples + 1, 0.0f));
        writeIndices.assign (numChannels, 0);
        
        envelope.assign (numChannels, 0.0f);
        smoothedGainReduction.assign (numChannels, 1.0f);
        
        reset();
    }

    void reset()
    {
        for (auto& buffer : lookaheadBuffer)
            std::fill (buffer.begin(), buffer.end(), 0.0f);
        
        std::fill (writeIndices.begin(), writeIndices.end(), 0);
        std::fill (envelope.begin(), envelope.end(), 0.0f);
        std::fill (smoothedGainReduction.begin(), smoothedGainReduction.end(), 1.0f);
    }

    float getGainReduction (int channel) const noexcept
    {
        int ch = std::clamp (channel, 0, static_cast<int> (smoothedGainReduction.size()) - 1);
        if (ch >= 0 && ch < smoothedGainReduction.size())
            return smoothedGainReduction[ch];
        return 1.0f;
    }

    /**
     * Process a stereo sample block
     */
    void processSample (float& leftSample, float& rightSample, float driveDb, float ceilingDb, float releaseMs, float mix, int mode) noexcept
    {
        if (numChannels <= 0)
            return;

        float driveGain = juce::Decibels::decibelsToGain (driveDb);
        float ceilingGain = juce::Decibels::decibelsToGain (ceilingDb);

        // Smooth envelope release coefficient calculations
        float releaseSec = std::max (releaseMs, 10.0f) / 1000.0f;
        float releaseCoef = std::exp (-1.0f / (static_cast<float> (sampleRate) * releaseSec));

        float inputSamples[2] = { leftSample * driveGain, rightSample * driveGain };

        for (int ch = 0; ch < 2; ++ch)
        {
            // Safeguard bounds for multi-channel scenarios
            int bufCh = std::min (ch, static_cast<int> (lookaheadBuffer.size()) - 1);
            if (bufCh < 0) continue;

            auto& delayLine = lookaheadBuffer[bufCh];
            int& writeIdx = writeIndices[bufCh];
            int delayLength = static_cast<int> (delayLine.size());

            float inVal = inputSamples[ch];

            // Push undelayed input to circular lookahead buffer
            delayLine[writeIdx] = inVal;

            // Read delayed sample (VCA input)
            int readIdx = (writeIdx + 1) % delayLength;
            float delayedSample = delayLine[readIdx];

            // Advance circular buffer index
            writeIdx = (writeIdx + 1) % delayLength;

            // --- Peak Detector Sidechain ---
            float absVal = std::abs (inVal);

            // Instant attack, smooth decay envelope tracking
            if (absVal > envelope[bufCh])
                envelope[bufCh] = absVal;
            else
                envelope[bufCh] = absVal + releaseCoef * (envelope[bufCh] - absVal);

            // Guard against subnormal values
            if (envelope[bufCh] < 1e-6f)
                envelope[bufCh] = 0.0f;

            // --- Gain Reduction Calculation ---
            float targetGain = 1.0f;
            if (envelope[bufCh] > 1e-5f)
            {
                if (mode == 0) // Clean Brickwall Mode
                {
                    if (envelope[bufCh] > ceilingGain)
                        targetGain = ceilingGain / envelope[bufCh];
                }
                else // LMC Squash Mode (High-ratio soft-knee compression)
                {
                    float envelopeDb = juce::Decibels::gainToDecibels (envelope[bufCh]);
                    float diffDb = envelopeDb - ceilingDb;
                    if (diffDb > 0.0f)
                    {
                        // Squish aggressively (20:1 ratio above threshold)
                        float compressedDb = ceilingDb + (diffDb / 20.0f);
                        targetGain = juce::Decibels::decibelsToGain (compressedDb) / envelope[bufCh];
                    }
                }
            }

            // Smooth VCA gain transitions to eliminate click/pop artifacts
            smoothedGainReduction[bufCh] += 0.25f * (targetGain - smoothedGainReduction[bufCh]);
            
            // NaN and Inf safety check
            if (std::isnan (smoothedGainReduction[bufCh]) || std::isinf (smoothedGainReduction[bufCh]))
                smoothedGainReduction[bufCh] = 1.0f;

            // NYC Parallel dry/wet crossfade
            float wetSample = delayedSample * smoothedGainReduction[bufCh];
            float outputSample = (1.0f - mix) * delayedSample + mix * wetSample;

            if (ch == 0) leftSample = outputSample;
            else rightSample = outputSample;
        }
    }

private:
    double sampleRate = 44100.0;
    int numChannels = 2;
    std::vector<std::vector<float>> lookaheadBuffer;
    std::vector<int> writeIndices;
    std::vector<float> envelope;
    std::vector<float> smoothedGainReduction;
};

} // namespace mushin
