#pragma once

#include "Filter.h"
#include "LFO.h"
#include <vector>

namespace mushin {

class DualFilterLFOMatrix {
public:
    enum class Routing {
        Serial,
        Parallel
    };

    DualFilterLFOMatrix() {
        // Initial setup
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        filterA.prepare(spec);
        filterB.prepare(spec);
        lfo1.prepare(spec);
        lfo2.prepare(spec);
        sampleRate = spec.sampleRate;
    }

    void reset() {
        filterA.reset();
        filterB.reset();
        lfo1.reset();
        lfo2.reset();
    }

    // --- Accessors for the Processor to update parameters ---
    Filter& getFilterA() { return filterA; }
    Filter& getFilterB() { return filterB; }
    LFO& getLFO1() { return lfo1; }
    LFO& getLFO2() { return lfo2; }

    void setRouting(Routing newRouting) { routing = newRouting; }

    // --- Modulation Matrix ---
    void setModAmount(int lfoIndex, int targetIndex, float amount) {
        if (lfoIndex >= 0 && lfoIndex < 2 && targetIndex >= 0 && targetIndex < 6) {
            modMatrix[lfoIndex][targetIndex] = amount;
        }
    }

    float processSample(int channel, float sample) {
        // 1. Calculate Modulation (Only update once per sample across all channels)
        // We use a simple trick: only update LFOs on channel 0
        if (channel == 0) {
            float lfo1Val = lfo1.getNextSample();
            float lfo2Val = lfo2.getNextSample();

            float modOffsets[6] = { 0.0f };
            for (int t = 0; t < 6; ++t) {
                modOffsets[t] = (lfo1Val * modMatrix[0][t]) + (lfo2Val * modMatrix[1][t]);
            }
            applyModulation(modOffsets);
        }

        // 2. Process Audio
        float output = 0.0f;
        if (routing == Routing::Serial) {
            float stage1 = filterA.processSample(channel, sample);
            output = filterB.processSample(channel, stage1);
        } else {
            float stageA = filterA.processSample(channel, sample);
            float stageB = filterB.processSample(channel, sample);
            output = (stageA + stageB) * 0.5f;
        }

        return output;
    }

    // Base values (set by UI/Processor)
    struct Params {
        float filterACutoff = 20000.0f;
        float filterAResonance = 0.1f;
        float filterAGrit = 0.0f;
        float filterBCutoff = 20000.0f;
        float filterBResonance = 0.1f;
        float filterBGrit = 0.0f;
    } baseParams;

private:
    void applyModulation(float offsets[6]) {
        auto applyCutoffMod = [](float baseFreq, float mod) {
            float octaves = mod * 5.0f; // +/- 5 octaves
            return std::clamp(baseFreq * std::pow(2.0f, octaves), 20.0f, 20000.0f);
        };

        filterA.setCutoff(applyCutoffMod(baseParams.filterACutoff, offsets[0]));
        filterA.setResonance(std::clamp(baseParams.filterAResonance + offsets[1], 0.0f, 1.0f));
        filterA.setGrit(std::clamp(baseParams.filterAGrit + offsets[2], 0.0f, 1.0f));

        filterB.setCutoff(applyCutoffMod(baseParams.filterBCutoff, offsets[3]));
        filterB.setResonance(std::clamp(baseParams.filterBResonance + offsets[4], 0.0f, 1.0f));
        filterB.setGrit(std::clamp(baseParams.filterBGrit + offsets[5], 0.0f, 1.0f));
    }

    Filter filterA, filterB;
    LFO lfo1, lfo2;
    Routing routing = Routing::Serial;
    double sampleRate = 44100.0;

    // modMatrix[Source][Target]
    float modMatrix[2][6] = { {0.0f} }; 
};

} // namespace mushin
