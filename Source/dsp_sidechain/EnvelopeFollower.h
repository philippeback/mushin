#pragma once

#include <juce_dsp/juce_dsp.h>

namespace mushin {

class EnvelopeFollower {
public:
    void prepare(double newSampleRate) {
        this->sampleRate = newSampleRate;
        reset();
    }

    void reset() {
        envelope = 0.0f;
    }

    void setParameters(float attackMs, float releaseMs, bool isRMS) {
        // Calculate coefficients for simple one-pole filter
        attack = (attackMs > 0.0f) ? std::exp(-1.0f / (float(sampleRate) * attackMs * 0.001f)) : 0.0f;
        release = (releaseMs > 0.0f) ? std::exp(-1.0f / (float(sampleRate) * releaseMs * 0.001f)) : 0.0f;
        rmsMode = isRMS;
    }

    float processSample(float input) {
        float x = std::abs(input);
        if (rmsMode) x = x * x;

        // Envelope following logic
        if (x > envelope) 
            envelope = attack * (envelope - x) + x;
        else              
            envelope = release * (envelope - x) + x;

        float out = rmsMode ? std::sqrt(std::max(0.0f, envelope)) : envelope;
        return std::clamp(out, 0.0f, 1.0f);
    }

    float getEnvelope() const { 
        return rmsMode ? std::sqrt(std::max(0.0f, envelope)) : envelope; 
    }

private:
    double sampleRate = 44100.0;
    float attack = 0.0f, release = 0.0f;
    float envelope = 0.0f;
    bool rmsMode = false;
};

} // namespace mushin
