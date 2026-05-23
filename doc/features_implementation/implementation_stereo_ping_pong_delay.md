# Implementation Design: Stereo Ping-Pong Delay

**Location:** `doc/features_implementation/implementation_stereo_ping_pong_delay.md`  
**Association:** `doc/features/feature_delay.md`  

---

## 1. Overview
The **Stereo Ping-Pong Delay** is a high-fidelity spatial time-domain processor integrated into Mushin's core audio engine. It features a tape-style 1-pole lowpass feedback damping filter, full host-BPM tempo synchronization, smoothed click-free parameter sweeps, and a compact glassmorphic top-line UI controller panel.

The processor supports two routing modes: standard stereo delay (independent Left and Right feedback paths) and Ping-Pong routing (cross-wired feedback loops that bounce echoes between Left and Right channels).

---

## 2. DSP Engine: `mushin::DelayProcessor`
The core delay DSP is encapsulated in `mushin::DelayProcessor` (located in `Source/dsp/DelayProcessor.h`). It relies on standard linear-interpolated JUCE delay lines and state variable TPT filters.

### 2.1 Technical and Mathematical Design
For left/right input samples $x_L, x_R$ at sample rate $f_s$, feedback coefficient $g_{fb} \in [0.0, 0.95]$, and mix coefficient $g_{mix} \in [0.0, 1.0]$:

1. **Tempo Sync and Time Translation**:
   When tempo-synchronized, the target delay time in milliseconds ($t_{ms}$) is computed from the host BPM ($BPM$) and the selected subdivision factor $F$ (where $F_{1/16} = 0.25, F_{1/8} = 0.5, F_{1/4} = 1.0, F_{1/2} = 2.0$):
   $$t_{ms} = \frac{60.0}{BPM} \times F \times 1000$$
   The continuous delay time in samples ($D_{samples}$) is derived and clamped to the maximum buffer capacity ($192,000$ samples):
   $$D_{samples} = \text{clamp}\left( \frac{t_{ms}}{1000} \times f_s, \ 0.0, \ 192000.0 - 1.0 \right)$$

2. **Damped Feedback Loops (TPT Filters)**:
   Feedback signals are routed through a 1-pole state variable TPT filter $H_{damp}(z)$ configured as a lowpass filter with a fixed vintage tape damping cutoff at $f_c = 2.5\text{kHz}$ to deliver warm, non-harsh repeats.

3. **Routing Topologies**:
   - **Standard Stereo**:
     $$d_L(n) = \text{DelayLine}_L(n)$$
     $$d_R(n) = \text{DelayLine}_R(n)$$
     $$\text{FeedbackInput}_L = x_L(n) + H_{damp}(d_L(n)) \times g_{fb}$$
     $$\text{FeedbackInput}_R = x_R(n) + H_{damp}(d_R(n)) \times g_{fb}$$
     $$\text{Push}_L(n) = \text{FeedbackInput}_L, \quad \text{Push}_R(n) = \text{FeedbackInput}_R$$
   - **Ping-Pong Delay (Cross-Channel Feedback)**:
     $$\text{Push}_L(n) = x_L(n) + H_{damp}(d_R(n)) \times g_{fb}$$
     $$\text{Push}_R(n) = x_R(n) + H_{damp}(d_L(n)) \times g_{fb}$$

4. **Dry/Wet Crossfade**:
   $$y_L(n) = (1.0 - g_{mix}) \times x_L(n) + g_{mix} \times d_L(n)$$
   $$y_R(n) = (1.0 - g_{mix}) \times x_R(n) + g_{mix} \times d_R(n)$$

### 2.2 C++ DSP Implementation
```cpp
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
```

---

## 3. Parameter Design (APVTS)
Seven parameters are registered under `createParameterLayout()` in `Source/PluginProcessor.cpp` for host-automation and preset state serialization:

| Parameter ID | Name | Type | Range | Default | Description |
|--------------|------|------|-------|---------|-------------|
| `delay_active` | Delay Active | Bool | [Off, On] | Off | Master bypass toggle for the Delay effect. |
| `delay_time` | Delay Time | Float | `1.0f` to `2000.0f` ms | `300.0f` ms | Continuous delay time (non-synchronized). |
| `delay_feedback` | Delay Feedback | Float | `0.0f` to `0.95f` | `0.3f` | Feedback gain level. |
| `delay_mix` | Delay Mix | Float | `0.0f` to `1.0f` | `0.3f` | Dry/Wet crossfade factor. |
| `delay_pingpong` | Ping-Pong | Bool | [Off, On] | Off | Cross-wires feedback channels when active. |
| `delay_sync` | Delay Sync | Bool | [Off, On] | Off | Locks delay repeats to host BPM subdivisions. |
| `delay_tempo` | Delay Tempo Sync | Choice | `["1/16", "1/8", "1/4", "1/2"]` | `"1/4"` | Subdivision selected for BPM synchronization. |

---

## 4. Integration into DSP Pipeline (`processBlock`)
The `DelayProcessor` is executed at **STAGE D.8** inside `processBlock()` in `Source/PluginProcessor.cpp`. In order to support cross-feedback channel calculations for Ping-Pong delay, the processor runs **outside the channel loop** but inside the sample loop.

To ensure completely smooth, click-free parameter sweeps, we apply standard 50ms linear smoothing ramps to delay parameters.

```cpp
// Inside processBlock() setup:
bool delayActive = (delayActiveParam->load() > 0.5f);
bool delayPingPong = (delayPingPongParam->load() > 0.5f);
bool delaySync = (delaySyncParam->load() > 0.5f);
int delayTempoVal = static_cast<int>(delayTempoParam->load());

smoothedDelayTime.setTargetValue(delayTimeParam->load());
smoothedDelayFeedback.setTargetValue(delayFeedbackParam->load());
smoothedDelayMix.setTargetValue(delayMixParam->load());

// Extract host playhead BPM
double bpm = 120.0;
if (auto* playHead = getPlayHead()) {
    if (auto pos = playHead->getPosition()) {
        if (auto optBpm = pos->getBpm()) {
            bpm = *optBpm;
        }
    }
}

// Inside unified sample loop:
for (int s = 0; s < numSamples; ++s)
{
    float scMod = smoothedScMeter.getNextValue();
    float currentDelayTime = smoothedDelayTime.getNextValue();
    float currentDelayFeedback = smoothedDelayFeedback.getNextValue();
    float currentDelayMix = smoothedDelayMix.getNextValue();

    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        // ... (Waveshaping, Filtering, Trance Gating, and Gain stages run here per-channel) ...
    }

    // --- STAGE D.8: Stereo Delay (outside channel loop, inside sample loop) ---
    if (delayActive && totalNumOutputChannels >= 2)
    {
        float leftSample = buffer.getSample(0, s);
        float rightSample = buffer.getSample(1, s);

        delayProcessor.processSample (leftSample, rightSample, currentDelayTime, currentDelayFeedback, currentDelayMix, delayPingPong, delaySync, (float)bpm, delayTempoVal);

        // NaN safety guard on delay output
        if (std::isnan(leftSample) || std::isinf(leftSample) || std::isnan(rightSample) || std::isinf(rightSample)) {
            leftSample = 0.0f;
            rightSample = 0.0f;
            reset();
        }

        buffer.setSample(0, s, leftSample);
        buffer.setSample(1, s, rightSample);
        
        // Push the echo-affected sample into oscilloscope FIFO
        pushNextSampleIntoFifo (leftSample);
    }
    else
    {
        // Fallback standard FIFO push
        pushNextSampleIntoFifo (buffer.getSample(0, s));
    }
}
```

---

## 5. Premium Glassmorphic Web UI
The Web UI features a premium compact Delay panel placed directly on the **top line** (`.top-section` Core Strip), nested immediately to the right of the `Gain` knob.

### 5.1 CSS Checked Toggle Animations
To match the premium look of Mushin's controls, we map the delay master bypass to a custom switch. The state transitions are styled in `Source/Web/index.html` to animate the power switch seamlessly:
```css
#qe_active:checked + .glow-switch-label,
#delay_active:checked + .glow-switch-label {
    border-color: var(--primary);
    background: rgba(0, 210, 255, 0.05);
}

#qe_active:checked + .glow-switch-label .glow-switch-dot,
#delay_active:checked + .glow-switch-label .glow-switch-dot {
    left: 11px;
    background: var(--primary);
    box-shadow: 0 0 6px var(--primary), inset 0 1px 1px rgba(255,255,255,0.8);
}
```

### 5.2 WebView2-Safe Interceptors & Bidirectional Sync
- Standard HSL toggle checkboxes (`display: none;`) suffer from event swallowing under Microsoft Edge/WebView2. To bypass this, a dedicated click interceptor is bound to `delay_active_indicator` in JavaScript to manually toggle the state and fire the `'change'` event back to the JUCE host.
- Bidirectional parameter sync translates continuous `0.0` - `1.0` normalized knob ranges to C++ parameter limits.
- When `Delay Sync` is toggled ON, the delay time knob dynamically swaps its readout text from millisecond limits (e.g. `300ms`) to active musical tempo divisions (e.g. `1/4`).

---

## 6. Verification
- **Compilation**: Verified and validated across both `Debug` and `Release` configurations under Windows.
- **Audio Routing**: Standard Stereo and Ping-Pong routing are audibly wide, click-free, and feedback-damped.
- **Visualization**: Oscilloscope reads delayed samples directly, allowing users to watch echo decay patterns in real-time.
