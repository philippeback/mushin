#pragma once

#include <juce_dsp/juce_dsp.h>

namespace mushin {

template <typename SampleType>
class LadderFilterExposed : public juce::dsp::LadderFilter<SampleType>
{
public:
    SampleType processSampleExposed (SampleType inputValue, size_t channelToUse) noexcept
    {
        return this->processSample (inputValue, channelToUse);
    }
};

class Filter {
public:
    enum class Type {
        Clean,
        Vintage,
        Acid,
        Digital
    };

    enum class Mode {
        Lowpass,
        Highpass,
        Bandpass,
        Notch
    };

    Filter() {
        cleanFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;
        cleanFilter.prepare(spec);
        ladderFilter.prepare(spec);
    }

    void reset() {
        cleanFilter.reset();
        ladderFilter.reset();
    }

    void setType(Type newType) {
        type = newType;
    }

    void setMode(Mode newMode) {
        mode = newMode;
        switch (newMode) {
            case Mode::Lowpass:
                cleanFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                ladderFilter.setMode(juce::dsp::LadderFilterMode::LPF12);
                break;
            case Mode::Highpass:
                cleanFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
                ladderFilter.setMode(juce::dsp::LadderFilterMode::HPF12);
                break;
            case Mode::Bandpass:
                cleanFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                ladderFilter.setMode(juce::dsp::LadderFilterMode::BPF12);
                break;
            case Mode::Notch:
                // TPT doesn't have notch, fallback to LP
                cleanFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
                break;
        }
    }

    void setCutoff(float newCutoff) {
        float clampedCutoff = std::clamp(newCutoff, 20.0f, 20000.0f);
        cleanFilter.setCutoffFrequency(clampedCutoff);
        ladderFilter.setCutoffFrequencyHz(clampedCutoff);
    }

    void setResonance(float newResonance) {
        float clampedResonance = std::clamp(newResonance, 0.0f, 1.0f);
        cleanFilter.setResonance(clampedResonance);
        ladderFilter.setResonance(clampedResonance);
    }

    void setDrive(float newDrive) {
        drive = newDrive;
    }

    void setGrit(float newGrit) {
        grit = newGrit;
    }

    float processSample(int channel, float sample) {
        // 1. Pre-filter Drive
        float x = sample * drive;
        x = std::tanh(x);

        // 2. Filter
        float filtered = 0.0f;
        if (type == Type::Vintage) {
            filtered = ladderFilter.processSampleExposed(x, (size_t)channel);
        } else {
            filtered = cleanFilter.processSample(channel, x);
        }

        // 3. Post-filter Grit
        float output = filtered * (1.0f + grit);
        output = std::tanh(output);

        return output;
    }

private:
    Type type = Type::Clean;
    Mode mode = Mode::Lowpass;
    double sampleRate = 44100.0;
    float drive = 1.0f;
    float grit = 0.0f;

    juce::dsp::StateVariableTPTFilter<float> cleanFilter;
    LadderFilterExposed<float> ladderFilter;
};

} // namespace mushin
