#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace mushin {

class NoiseOscillator {
public:
    enum class Type {
        WhiteNoise = 0,
        PinkNoise,
        Sine,
        Triangle,
        Saw,
        Square
    };

    NoiseOscillator() {
        randomGenerator.setSeed(juce::Random::getSystemRandom().nextInt());
        reset();
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
        reset();
    }

    void reset() {
        phase = 0.0f;
        pink_b0 = 0.0f;
        pink_b1 = 0.0f;
        pink_b2 = 0.0f;
        pink_b3 = 0.0f;
        pink_b4 = 0.0f;
        pink_b5 = 0.0f;
        pink_b6 = 0.0f;
    }

    void setType(int newType) {
        type = static_cast<Type>(juce::jlimit(0, 5, newType));
    }

    void setFrequency(float newFrequency) {
        frequency = juce::jlimit(20.0f, 20000.0f, newFrequency);
    }

    float nextSample() {
        // Advance phase for oscillator waveforms
        phase += frequency / static_cast<float>(sampleRate);
        if (phase >= 1.0f) {
            phase -= 1.0f;
        }

        switch (type) {
            case Type::WhiteNoise: {
                return (randomGenerator.nextFloat() * 2.0f) - 1.0f;
            }
            case Type::PinkNoise: {
                float white = (randomGenerator.nextFloat() * 2.0f) - 1.0f;
                pink_b0 = 0.99886f * pink_b0 + white * 0.0555179f;
                pink_b1 = 0.99332f * pink_b1 + white * 0.0750759f;
                pink_b2 = 0.96900f * pink_b2 + white * 0.1538520f;
                pink_b3 = 0.86650f * pink_b3 + white * 0.3104856f;
                pink_b4 = 0.55000f * pink_b4 + white * 0.5329522f;
                pink_b5 = -0.7616f * pink_b5 - white * 0.0168980f;
                float pink = pink_b0 + pink_b1 + pink_b2 + pink_b3 + pink_b4 + pink_b5 + pink_b6 + white * 0.5362f;
                pink_b6 = white * 0.115926f;
                return pink * 0.11f; // Normalize output scale to match white noise volume
            }
            case Type::Sine: {
                return std::sin(phase * juce::MathConstants<float>::twoPi);
            }
            case Type::Triangle: {
                if (phase < 0.25f) return 4.0f * phase;
                if (phase < 0.75f) return 2.0f - 4.0f * phase;
                return 4.0f * phase - 4.0f;
            }
            case Type::Saw: {
                return 2.0f * phase - 1.0f;
            }
            case Type::Square: {
                return (phase < 0.5f) ? 1.0f : -1.0f;
            }
        }
        return 0.0f;
    }

private:
    Type type = Type::WhiteNoise;
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    float phase = 0.0f;

    // Pink noise filter state (Paul Kellet's refined 3-decade approximation)
    float pink_b0 = 0.0f;
    float pink_b1 = 0.0f;
    float pink_b2 = 0.0f;
    float pink_b3 = 0.0f;
    float pink_b4 = 0.0f;
    float pink_b5 = 0.0f;
    float pink_b6 = 0.0f;

    juce::Random randomGenerator;
};

} // namespace mushin
