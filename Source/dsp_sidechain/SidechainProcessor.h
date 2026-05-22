#pragma once

#include <juce_dsp/juce_dsp.h>
#include "EnvelopeFollower.h"

namespace mushin {

class SidechainProcessor {
public:
    enum class Source {
        Internal,
        External,
        TestSignal
    };

    enum class Target {
        Drive,
        Cutoff,
        Gain
    };

    SidechainProcessor() {
        hpFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        lpFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        currentSampleRate = spec.sampleRate;
        
        envelopeFollower.prepare(currentSampleRate);
        
        filterSpec.sampleRate = currentSampleRate;
        filterSpec.maximumBlockSize = spec.maximumBlockSize;
        filterSpec.numChannels = 1; // Processing sidechain as mono
        
        hpFilter.prepare(filterSpec);
        lpFilter.prepare(filterSpec);
        
        updateFilters();
    }

    void reset() {
        envelopeFollower.reset();
        hpFilter.reset();
        lpFilter.reset();
    }

    void setParameters(float attackMs, float releaseMs, bool isRMS, float thresholdDb, float newAmount, float newHpFreq, float newLpFreq) {
        envelopeFollower.setParameters(attackMs, releaseMs, isRMS);
        threshold = juce::Decibels::decibelsToGain(thresholdDb);
        this->amount = newAmount;
        
        bool filtersChanged = (this->hpFreq != newHpFreq || this->lpFreq != newLpFreq);
        this->hpFreq = newHpFreq;
        this->lpFreq = newLpFreq;
        
        if (filtersChanged) {
            updateFilters();
        }
    }

    void setSource(Source newSource) { source = newSource; }
    Source getSource() const { return source; }
    
    void setTarget(Target newTarget) { target = newTarget; }
    Target getTarget() const { return target; }

    void setActive(bool isActive) { active = isActive; }
    bool isActive() const { return active; }

    float processSample(float sidechainSample) {
        if (!active) return 0.0f;

        // 1. Filter the sidechain signal
        float filtered = hpFilter.processSample(0, sidechainSample);
        filtered = lpFilter.processSample(0, filtered);

        // 2. Get envelope
        float env = envelopeFollower.processSample(filtered);

        // 3. Calculate modulation (threshold crossing)
        // If env > threshold, calculate how much it's over
        float modulation = 0.0f;
        if (env > threshold) {
            modulation = (env - threshold) / (1.0f - threshold + 0.0001f);
        }

        return modulation * amount;
    }

private:
    void updateFilters() {
        hpFilter.setCutoffFrequency(std::clamp(hpFreq, 20.0f, 20000.0f));
        lpFilter.setCutoffFrequency(std::clamp(lpFreq, 20.0f, 20000.0f));
    }

    double currentSampleRate = 44100.0;
    juce::dsp::ProcessSpec filterSpec;
    
    EnvelopeFollower envelopeFollower;
    juce::dsp::StateVariableTPTFilter<float> hpFilter;
    juce::dsp::StateVariableTPTFilter<float> lpFilter;
    
    Source source = Source::Internal;
    Target target = Target::Gain;
    bool active = false;
    float threshold = 0.0f;
    float amount = 0.0f;
    float hpFreq = 20.0f;
    float lpFreq = 20000.0f;
};

} // namespace mushin
