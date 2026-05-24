# Feature: Final Stage Limiter / LMC (Listen Mic Compressor)

This document details the functional specifications, DSP architecture, and UI requirements for the final-stage **Limiter / LMC** module in the Mushin synthesizer.

---

## 1. Overview & Dual Philosophy

To guarantee audio safety during heavy waveshaping and feedback loops, and to provide a cohesive "glue" to the final mixed output, a dedicated **Limiter/LMC** stage is integrated at the very end of the DSP signal path (post-Stage E Final Gain).

The module operates in two distinct functional modes, selectable by the user:

### Mode A: Clean (Safety Brickwall)
A highly transparent, zero-overshoot peak limiter.
* **Purpose**: Prevents digital clipping (keeping output safely below `0.0 dBFS` or a user-defined ceiling) while preserving transient clarity.
* **DSP Profile**: Ultra-fast lookahead detection, hard-knee thresholding, and transparent, low-distortion envelope recovery.

### Mode B: LMC (Listen Mic Compressor)
An aggressive, highly-colored character compressor inspired by the legendary **SSL Listen Mic Compressor (LMC)**.
* **Purpose**: Adds dramatic "glue," pumping, and aesthetic grit to the synthesizer's output. Perfect for parallel compression (NYC-style smashing) and explosive transients.
* **DSP Profile**: Fixed instantaneous attack, soft-knee high-ratio compression, grabby feedback envelope tracking, and subtle harmonically-rich gain-reduction saturation.

---

## 2. Parameter Specifications (APVTS)

The Limiter/LMC module is fully automated and exposes 6 parameters to the JUCE APVTS:

| Parameter ID | Display Name | Type | Range | Default | Description |
|---|---|---|---|---|---|
| `limiter_active` | Limiter Active | Bool | `[OFF, ON]` | `OFF` | Bypass toggle. |
| `limiter_mode` | Limiter Mode | Choice | `[Clean, LMC]` | `Clean` | Swaps between Transparent and Color modes. |
| `limiter_drive` | Limiter Drive | Float | `[0.0, +24.0] dB` | `0.0 dB` | Input drive to push signals into the threshold. |
| `limiter_ceiling` | Limit Ceiling | Float | `[-12.0, 0.0] dBFS` | `-0.1 dBFS` | Maximum peak output level. |
| `limiter_release` | Limit Release | Float | `[10.0, 1000.0] ms` | `100.0 ms` | Envelope release duration (with Auto option). |
| `limiter_mix` | Limiter Mix | Float | `[0.0, 1.0]` | `1.0` | Parallel compression mix (Dry/Wet). |

---

## 3. DSP Architecture (C++)

To implement a true brickwall peak limiter, the DSP requires a **Lookahead Delay Buffer** to anticipate peaks before they occur.

```
                  [ Lookahead Delay Line (e.g., 2.0 ms) ]
Input Signal ----> [====================================] ----> [ VCA Gain Multiplier ] ---> Output
         \                                                            ^
          \---> [ Peak Detector ] -> [ Sidechain Gain Reduction ] ---/
```

### Lookahead circular buffer
* **Lookahead Time**: `2.0 ms` (approximately 88 samples at 44.1kHz).
* **Operation**: The incoming signal is written to a circular buffer and read back with a 2.0ms delay. The Peak Detector analyzes the *undelayed* signal, allowing the envelope tracker to ramp up gain reduction *before* the transient peak actually arrives at the VCA multiplier, guaranteeing **zero overshoot**.

### Gain Reduction Curves
* **Clean Mode (Brickwall)**:
  $$G(t) = \min\left(1.0, \frac{\text{Ceiling}}{\text{Envelope}(t)}\right)$$
  Using a hard-knee curve to enforce absolute peaks.
* **LMC Mode (Compression)**:
  Utilizes a standard compressor sidechain with a soft-knee and high compression ratio ($20:1$). The release curve features a non-linear decay that models the "grab and pump" of the vintage analog SSL talkback circuit.

---

## 4. C++ Implementation Layout

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace mushin {

class LimiterProcessor
{
public:
    LimiterProcessor() = default;
    ~LimiterProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        
        // Allocate lookahead buffers (2.0ms max lookahead at 96kHz)
        int lookaheadSamples = static_cast<int>((2.0f / 1000.0f) * sampleRate);
        lookaheadBuffer.resize(spec.numChannels, std::vector<float>(lookaheadSamples + 1, 0.0f));
        writeIndices.assign(spec.numChannels, 0);
        
        // Reset envelope states
        envelope.assign(spec.numChannels, 0.0f);
        smoothedGainReduction.assign(spec.numChannels, 1.0f);
    }

    void reset()
    {
        for (auto& buffer : lookaheadBuffer)
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        
        std::fill(writeIndices.begin(), writeIndices.end(), 0);
        std::fill(envelope.begin(), envelope.end(), 0.0f);
        std::fill(smoothedGainReduction.begin(), smoothedGainReduction.end(), 1.0f);
    }

    float getGainReduction(int channel) const
    {
        if (channel < smoothedGainReduction.size())
            return smoothedGainReduction[channel];
        return 1.0f;
    }

    void process(juce::AudioBuffer<float>& buffer, 
                 float driveDb, 
                 float ceilingDb, 
                 float releaseMs, 
                 float mix, 
                 int mode)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        float driveGain = juce::Decibels::decibelsToGain(driveDb);
        float ceilingGain = juce::Decibels::decibelsToGain(ceilingDb);
        
        // Calculate filter coefficients for envelope release
        float releaseSec = releaseMs / 1000.0f;
        float releaseCoef = std::exp(-1.0f / (sampleRate * releaseSec));
        float attackCoef = 0.0f; // Instantaneous attack for brickwall safety

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            auto& delayLine = lookaheadBuffer[ch];
            int& writeIdx = writeIndices[ch];
            int delayLength = delayLine.size();

            for (int s = 0; s < numSamples; ++s)
            {
                float inputSample = channelData[s] * driveGain;
                
                // Push undelayed input to circular lookahead buffer
                delayLine[writeIdx] = inputSample;
                
                // Read delayed sample
                int readIdx = (writeIdx + 1) % delayLength;
                float delayedSample = delayLine[readIdx];
                
                // Advance circular index
                writeIdx = (writeIdx + 1) % delayLength;

                // --- Peak Detector Sidechain ---
                float absVal = std::abs(inputSample);
                
                // Peak envelope tracker
                if (absVal > envelope[ch])
                    envelope[ch] = absVal; // Instant attack
                else
                    envelope[ch] = absVal + releaseCoef * (envelope[ch] - absVal); // Smooth release

                // --- Gain Reduction Calculation ---
                float targetGain = 1.0f;
                if (mode == 0) // Clean Mode
                {
                    if (envelope[ch] > ceilingGain)
                        targetGain = ceilingGain / envelope[ch];
                }
                else // LMC Vibe Mode
                {
                    // High-ratio soft-knee compression curve
                    float diffDb = juce::Decibels::gainToDecibels(envelope[ch]) - ceilingDb;
                    if (diffDb > 0.0f)
                    {
                        // Compress heavily (20:1 ratio) above threshold
                        float compressedDb = ceilingDb + (diffDb / 20.0f);
                        targetGain = juce::Decibels::decibelsToGain(compressedDb) / envelope[ch];
                    }
                }

                // Smooth VCA gain transition to eliminate pops
                smoothedGainReduction[ch] += 0.15f * (targetGain - smoothedGainReduction[ch]);

                // Dry/Wet nyc-style parallel mixing
                float wetSample = delayedSample * smoothedGainReduction[ch];
                channelData[s] = (1.0f - mix) * delayedSample + mix * wetSample;
            end
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<std::vector<float>> lookaheadBuffer;
    std::vector<int> writeIndices;
    std::vector<float> envelope;
    std::vector<float> smoothedGainReduction;
};

} // namespace mushin
```

---

## 5. UI Layout & Visualizer Specifications

The Limiter is hosted in a premium glassmorphic `.sub-panel` inside the top line core strip, positioned post-Delay:

1. **LED Gain Reduction Ladder**:
   * A vertical 10-segment LED ladder tracking `smoothedGainReduction`.
   * **Color Scale**: Cyan (`0 dB` to `-3 dB`) transitioning to Orange (`-4 dB` to `-9 dB`) and glowing Red (`-10 dB` and beyond).
   * Pushed to the WebView2 UI at 30Hz using standard C++ event-emission hooks.
2. **Moog Knobs**:
   * **Drive**: Adjusts input push into the threshold.
   * **Ceiling**: Sets maximum hard/soft peak output limits.
   * **Release**: Continuous release envelope speed (includes an `AUTO` toggle switch).
3. **NYC Parallel Mix**:
   * Standard `.mushin-knob` dry/wet blend for beautiful parallel compression squash.
