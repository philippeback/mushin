# Implementation Design: Quantization Error & Information Decay

**Location:** `doc/features_implementation/implementation_quantization_error.md`  
**Association:** `doc/features/feature_quantizatiion_error.md`  

---

## 1. Overview
The **Quantization Error (Information Decay)** module simulates the degradation, reduction in amplitude resolution, and temporal decimation of audio data as statistical confidence in a financial trend collapses. In Mushin's narrative, high confidence corresponds to ultra-high 24-bit resolution, while trend exhaustion rapidly decimates the signal down to low-fidelity 4-bit or 2-bit formats.

The module provides continuous bit-depth modulation with sample-rate downsampling, dynamic smoothing to prevent mechanical switching clicks, and direct integration with the global **Exhaustion** state.

---

## 2. DSP Engine: `mushin::QuantizationErrorProcessor`
A new C++ class, `mushin::QuantizationErrorProcessor` (located in `Source/dsp/QuantizationErrorProcessor.h`), generates the bitcrushing and downsampling effects.

### 2.1 The Mathematical Formulation
For a bipolar floating-point sample $x \in [-1.0, 1.0]$, a continuous bit-depth $B \in [2.0, 24.0]$, downsample factor $D \in [1, 32]$, and mix $M \in [0.0, 1.0]$:

1. **Time-Domain Downsampling (Rate Decimation)**:
   - A sample counter track $c$ advances with each processed sample.
   - The output sample is updated only when $c \pmod D == 0$; otherwise, the processor holds and outputs the previous sample.

2. **Amplitude-Domain Quantization (Mid-Tread)**:
   - Continuous Step Scale ($S$):
     $$S = 2^{B - 1}$$
   - Bipolar mid-tread quantization:
     $$y = \frac{\text{std::round}(x \cdot S)}{S}$$

3. **Dry/Wet Crossfade**:
   - The output is linearly crossfaded between the clean sample and the processed sample:
     $$\text{out} = (1.0 - M) \cdot x + M \cdot y$$

### 2.2 C++ Processor Implementation
```cpp
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <algorithm>

namespace mushin {

class QuantizationErrorProcessor
{
public:
    QuantizationErrorProcessor() = default;
    ~QuantizationErrorProcessor() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            heldSample[ch] = 0.0f;
            sampleCounter[ch] = 0;
        }
    }

    float processSample(int channel, float sample, float bitDepth, int downsample, float mix) noexcept
    {
        if (mix <= 0.001f)
            return sample;

        float wetSample = sample;

        // --- 1. Time-Domain Resolution Decay (Downsampling) ---
        if (downsample > 1)
        {
            if (sampleCounter[channel] % downsample == 0)
            {
                heldSample[channel] = wetSample;
            }
            wetSample = heldSample[channel];
            sampleCounter[channel]++;
        }
        else
        {
            sampleCounter[channel] = 0;
        }

        // --- 2. Amplitude-Domain Resolution Decay (Quantization) ---
        if (bitDepth < 23.9f)
        {
            float safeDepth = std::max(2.0f, bitDepth);
            float steps = std::pow(2.0f, safeDepth - 1.0f);
            wetSample = std::round(wetSample * steps) / steps;
        }

        // --- 3. Dry/Wet Crossfade ---
        return (1.0f - mix) * sample + mix * wetSample;
    }

private:
    double currentSampleRate = 44100.0;
    float heldSample[2] = { 0.0f, 0.0f };
    uint32_t sampleCounter[2] = { 0, 0 };
};

} // namespace mushin
```

---

## 3. Parameter Design (APVTS)
Five new parameters are registered under `createParameterLayout()` in `Source/PluginProcessor.cpp` for preset saving/loading and UI synchronization:

| Parameter ID | Name | Type | Range | Default | Description |
|--------------|------|------|-------|---------|-------------|
| `qe_active` | Info Decay Active | Bool | [Off, On] | Off | Master bypass toggle for the quantization effect. |
| `qe_depth` | Decayed Resolution | Float | `2.0f` to `24.0f` bits | `24.0f` bits | Target resolution depth (amplitude domain). |
| `qe_downsample` | Time Resolution | Float | `1.0f` to `32.0f` | `1.0f` | Downsampling factor (time domain). |
| `qe_mix` | Decay Mix | Float | `0.0f` to `1.0f` | `1.0f` | Dry/Wet crossfade factor. |
| `qe_link` | Exhaustion Link | Bool | [Off, On] | Off | Links bit depth directly to global exhaustion status. |

---

## 4. Integration into DSP Pipeline (`processBlock`)
The processor is integrated into `processBlock()` at **STAGE A.5** (immediately post-distortion, pre-filtering). To guarantee click-free parameter modulation, smoothed values with 50ms response times are used.

```cpp
// Inside processBlock() setup:
bool qeActive = (qeActiveParam->load() > 0.5f);
float targetQeDepth = qeDepthParam->load();

// If link is ON and global exhaustion is triggered, pull depth down dynamically
if (qeLinkParam->load() > 0.5f && exhaustionParam->load() > 0.5f) {
    targetQeDepth = 4.0f; // Rapid collapse to gritty 4-bit resolution
}

smoothedQeDepth.setTargetValue(targetQeDepth);
smoothedQeMix.setTargetValue(qeMixParam->load());
int downsampleFactor = static_cast<int>(qeDownsampleParam->load());

// Inside unified sample loop (advanced once per sample to avoid channel-advancement bugs):
for (int s = 0; s < numSamples; ++s)
{
    float currentQeDepth = smoothedQeDepth.getNextValue();
    float currentQeMix = smoothedQeMix.getNextValue();

    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        // ... STAGE A: Waveshaping Distortion ...
        wetSample = waveshaper.processSample(ch, wetSample);

        // --- STAGE A.5: Quantization Error (Information Decay) ---
        if (qeActive) {
            wetSample = quantizationError.processSample(ch, wetSample, currentQeDepth, downsampleFactor, currentQeMix);
        }

        // ... STAGE B: Dual Filtering ...
    }
}
```

---

## 5. Premium Glassmorphic Web UI
The Web UI features a custom retro-sci-fi glassmorphic panel in Column 3 with metallic knobs, real-time warning indicators, and canvas rendering sweeps.

### 5.1 Relocated Noise Panel & Squeezed Layout
To make perfect room for `INFO DECAY` in Column 3, the `NOISE / GENERATOR` sub-panel has been moved to Column 1 (under the filters) and squeezed to fit:
- Spacial optimization: `Active`, `Routing`, and `Type` elements are merged into a single compact header line.
- Reduced sizes: Squeezed padding, margins, and option labels to ensure a tight, no-overflow fit.

### 5.2 WebView2-Safe Clicks & Bidirectional Sync
- A dedicated Javascript click listener was attached to the `qe_active_indicator` label to reliably toggle the hidden C++ checkbox and fire the standard `'change'` event, bypassing WebView2 click-through bugs.
- Bidirectional parameter sync is registered in the JS `params` dictionary mapping `0.0` - `1.0` normalized values to the `2.0` - `24.0` C++ bit range.

### 5.3 CRT Glitch Scanlines & Stairstep Oscilloscope
- **Scanlines**: Panel classes automatically add a `.glitching-panel` scanline CRT keyframe animation when the system resolution falls below **8.0 bits**.
- **oscilloscope stairsteps**: The canvas render loop automatically shifts from drawing smooth lines to sharp digital "stairstep" segments when Info Decay is active and bit depth drops below **10.0 bits**.
