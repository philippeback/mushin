#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

namespace mushin {

class DelayProcessor
{
public:
    DelayProcessor()
    {
        // Max delay time of 2.0 seconds at 96kHz is around 192000 samples
        delayLineL.setMaximumDelayInSamples (192000);
        delayLineR.setMaximumDelayInSamples (192000);
    }

    ~DelayProcessor() = default;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        delayLineL.prepare (spec);
        delayLineR.prepare (spec);
        
        feedbackFilterL.prepare (spec);
        feedbackFilterR.prepare (spec);
        
        feedbackFilterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        feedbackFilterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        
        // Damp higher frequencies in the feedback loop around 2.5kHz for vintage tape vibe
        feedbackFilterL.setCutoffFrequency (2500.0f);
        feedbackFilterR.setCutoffFrequency (2500.0f);

        reset();
    }

    void reset()
    {
        delayLineL.reset();
        delayLineR.reset();
        feedbackFilterL.reset();
        feedbackFilterR.reset();
        lastFeedbackL = 0.0f;
        lastFeedbackR = 0.0f;
    }

    /**
     * Process a stereo sample block
     */
    void processSample (float& leftSample, float& rightSample, float timeMs, float feedback, float mix, bool pingpong, bool sync, float syncBpm, int syncType) noexcept
    {
        if (mix <= 0.001f)
            return;

        // Calculate time in samples
        float targetTimeMs = timeMs;
        if (sync)
        {
            double bpm = syncBpm > 1.0f ? syncBpm : 120.0;
            double quarterNoteSec = 60.0 / bpm;
            double factor = 1.0;
            switch (syncType)
            {
                case 0: factor = 0.25; break; // 1/16
                case 1: factor = 0.5;  break; // 1/8
                case 2: factor = 1.0;  break; // 1/4
                case 3: factor = 2.0;  break; // 1/2
            }
            targetTimeMs = static_cast<float> (quarterNoteSec * factor * 1000.0);
        }

        // Smooth time target updates inside the delay lines
        float delaySamples = (targetTimeMs * 0.001f) * static_cast<float> (sampleRate);
        
        // Safety clamp
        delaySamples = std::clamp (delaySamples, 0.0f, 192000.0f - 1.0f);

        delayLineL.setDelay (delaySamples);
        delayLineR.setDelay (delaySamples);

        // Fetch delayed samples
        float delayedL = delayLineL.popSample (0);
        float delayedR = delayLineR.popSample (0);

        // Feedback loop with lowpass damping
        float fbInputL = leftSample + lastFeedbackL * feedback;
        float fbInputR = rightSample + lastFeedbackR * feedback;

        // Push to delay lines
        if (pingpong)
        {
            // In pingpong, feedback cross-wires the channels
            delayLineL.pushSample (0, leftSample + lastFeedbackR * feedback);
            delayLineR.pushSample (0, rightSample + lastFeedbackL * feedback);
        }
        else
        {
            delayLineL.pushSample (0, fbInputL);
            delayLineR.pushSample (0, fbInputR);
        }

        // Apply lowpass filter to feedback path for nice tape damping
        lastFeedbackL = feedbackFilterL.processSample (0, delayedL);
        lastFeedbackR = feedbackFilterR.processSample (0, delayedR);

        // Dry/Wet crossfade
        leftSample = (1.0f - mix) * leftSample + mix * delayedL;
        rightSample = (1.0f - mix) * rightSample + mix * delayedR;
    }

private:
    double sampleRate = 44100.0;
    juce::dsp::DelayLine<float> delayLineL { 192000 };
    juce::dsp::DelayLine<float> delayLineR { 192000 };

    juce::dsp::StateVariableTPTFilter<float> feedbackFilterL;
    juce::dsp::StateVariableTPTFilter<float> feedbackFilterR;

    float lastFeedbackL = 0.0f;
    float lastFeedbackR = 0.0f;
};

} // namespace mushin
