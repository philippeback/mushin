#pragma once

#include <juce_dsp/juce_dsp.h>

namespace mushin {

class LFO {
public:
    enum class Waveform {
        Sine,
        Triangle,
        Saw,
        Square,
        Random
    };

    LFO() {
        updateWaveform();
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
        osc.prepare(spec);
        
        // Setup Random S&H
        randomGenerator.setSeed(juce::Random::getSystemRandom().nextInt());
    }

    void reset() {
        osc.reset();
        phase = 0.0f;
    }

    void setWaveform(Waveform newWaveform) {
        if (waveform != newWaveform) {
            waveform = newWaveform;
            updateWaveform();
        }
    }

    void setFrequency(float newFrequency) {
        osc.setFrequency(newFrequency);
        frequency = newFrequency;
    }

    float getNextSample() {
        if (waveform == Waveform::Random) {
            phase += frequency / (float)sampleRate;
            if (phase >= 1.0f) {
                phase -= 1.0f;
                lastRandomValue = (randomGenerator.nextFloat() * 2.0f) - 1.0f;
            }
            return lastRandomValue;
        }
        
        return osc.processSample(0.0f);
    }

private:
    void updateWaveform() {
        switch (waveform) {
            case Waveform::Sine:
                osc.initialise([](float x) { return std::sin(x); });
                break;
            case Waveform::Triangle:
                osc.initialise([](float x) {
                    return (x < 0) ? (x * juce::MathConstants<float>::twoPi + 1.0f) 
                                   : (1.0f - x * juce::MathConstants<float>::twoPi);
                });
                // Note: juce::dsp::Oscillator input is in radians from -pi to pi usually?
                // Actually it depends on how it's used. Standard is sin(x) where x is -pi to pi.
                // For Triangle:
                osc.initialise([](float x) {
                    float val = x / juce::MathConstants<float>::pi; // -1 to 1
                    if (val < -0.5f) return 2.0f * (val + 1.0f) - 1.0f;
                    if (val < 0.5f) return -2.0f * val;
                    return 2.0f * (val - 1.0f) - 1.0f;
                    // Let's use a simpler one
                });
                osc.initialise([](float x) {
                    return 2.0f * std::abs(2.0f * (x / juce::MathConstants<float>::twoPi + 0.5f) - std::floor(x / juce::MathConstants<float>::twoPi + 0.5f + 0.5f)) - 1.0f;
                });
                break;
            case Waveform::Saw:
                osc.initialise([](float x) { return x / juce::MathConstants<float>::pi; });
                break;
            case Waveform::Square:
                osc.initialise([](float x) { return (x < 0) ? -1.0f : 1.0f; });
                break;
            case Waveform::Random:
                // Handled in getNextSample
                break;
        }
    }

    juce::dsp::Oscillator<float> osc;
    Waveform waveform = Waveform::Sine;
    double sampleRate = 44100.0;
    float frequency = 1.0f;
    float phase = 0.0f;
    float lastRandomValue = 0.0f;
    juce::Random randomGenerator;
};

} // namespace mushin
