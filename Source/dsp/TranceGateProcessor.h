#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

namespace mushin {

struct TranceGatePattern {
    juce::String name;
    uint16_t mask;
};

static const std::vector<TranceGatePattern> presetPatterns = {
    { "Straight 16th", 0xAAAA },     // 1010101010101010
    { "Offbeat 16th",  0x5555 },     // 0101010101010101
    { "Classic 1",     0xEEEE },     // 1110111011101110
    { "Classic 2",     0x9999 },     // 1001100110011001
    { "Four-On-Floor", 0xF0F0 },     // 1111000011110000
    { "Galop",         0xD7D7 },     // 1101011111010111
    { "Space Gate",    0x8888 },     // 1000100010001000
    { "Euclidean 5",   0x8912 }      // 1000100100010010
};

class TranceGateProcessor
{
public:
    TranceGateProcessor() = default;
    ~TranceGateProcessor() = default;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        currentSamplePosition = 0.0;
        currentStepIndex.store(-1);
    }

    void reset()
    {
        currentSamplePosition = 0.0;
        currentStepIndex.store(-1);
    }

    float processSample (double sPos, double stepDurationSamples, double cycleDurationSamples,
                         uint16_t patternMask, double holdWidthSamples, double attackEnd, double decayStart,
                         float baseAtten, float tgMix)
    {
        double sCycle = std::fmod (sPos, cycleDurationSamples);
        if (sCycle < 0.0) sCycle += cycleDurationSamples;

        int step = (int)(sCycle / stepDurationSamples);
        if (step < 0) step = 0;
        else if (step > 15) step = 15;

        currentStepIndex.store(step);

        double sOffset = sCycle - (step * stepDurationSamples);

        bool stepActive = (patternMask & (1 << step)) != 0;

        float envVal = 0.0f;
        if (stepActive)
        {
            if (sOffset < attackEnd && attackEnd > 0.0)
            {
                envVal = (float)(sOffset / attackEnd);
            }
            else if (sOffset < decayStart)
            {
                envVal = 1.0f;
            }
            else if (sOffset < holdWidthSamples && (holdWidthSamples - decayStart) > 0.0)
            {
                double transitionOffset = sOffset - decayStart;
                envVal = 1.0f - (float)(transitionOffset / (holdWidthSamples - decayStart));
            }
            else
            {
                envVal = 0.0f;
            }
        }

        float sampleGain = baseAtten + ((1.0f - baseAtten) * envVal);
        return (1.0f - tgMix) + (tgMix * sampleGain);
    }

    void advanceFreeRunningClock (int numSamples, double cycleDurationSamples)
    {
        currentSamplePosition = std::fmod (currentSamplePosition + numSamples, cycleDurationSamples);
    }

    double getCurrentSamplePosition() const { return currentSamplePosition; }
    void setCurrentSamplePosition (double newPos) { currentSamplePosition = newPos; }

    int getCurrentStep() const { return currentStepIndex.load(); }

private:
    double sampleRate = 44100.0;
    double currentSamplePosition = 0.0;
    std::atomic<int> currentStepIndex { -1 };
};

} // namespace mushin
