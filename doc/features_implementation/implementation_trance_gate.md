# Technical Implementation Specification: Tempo-Synced Trance Gate DSP & UI

**Location:** `doc/features_implementation/implementation_trance_gate.md`  
**Association:** `doc/features/feature_trancegating.md`  
**Date:** May 2026

---

## 1. Overview & Architecture

The **Trance Gate** is a pattern-based volume gating effect that applies a rhythmic volume envelope to the synthesizer's output. It synchronizes with the host's tempo (BPM) to create classic rhythmic "trance gating" effects.

```mermaid
graph TD
    DAW[DAW Host Playhead] -->|BPM & Playhead Pos| Sync[Tempo Synchronization]
    Sync -->|Current Sample Pos| StepCalc[Step & Phase Calculator]
    StepCalc -->|Current Step & Offset| EnvGen[Volume Contour Envelope Generator]
    PatternBank[Preset Pattern Bank] -->|Pattern Grid| EnvGen
    AudioInput[Wet Audio Signal] -->|Volume Scaling| Atten[Gating Attenuator]
    EnvGen -->|Envelope Gain| Atten
    Atten -->|Dry/Wet Crossfade| Output[Final Gated Output]
```

---

## 2. DSP Engine Architecture

The Trance Gate processor will be implemented as a dedicated DSP utility in `Source/dsp/TranceGateProcessor.h`.

### 2.1 Pattern Bitmask Representation
To achieve maximum speed and zero memory overhead, the 16-step patterns are represented as a sequence of `uint16_t` bitmasks. Step 0 corresponds to the Least Significant Bit (LSB) and Step 15 to the Most Significant Bit (MSB).

```cpp
namespace mushin {
struct TranceGatePattern {
    juce::String name;
    uint16_t mask;
};

static const std::vector<TranceGatePattern> presetPatterns = {
    { "Straight 16th", 0xAAAA },     // 1010101010101010 (Alternate)
    { "Offbeat 16th",  0x5555 },     // 0101010101010101 (Offbeat)
    { "Classic 1",     0xEEEE },     // 1110111011101110 (Trance Classic A)
    { "Classic 2",     0x9999 },     // 1001100110011001 (Syncopated)
    { "Four-On-Floor", 0xF0F0 },     // 1111000011110000
    { "Galop",         0xD7D7 },     // 1101011111010111
    { "Space Gate",    0x8888 },     // 1000100010001000 (Staccato)
    { "Euclidean 5",   0x8912 }      // 1000100100010010
};
}
```

### 2.2 Tempo Synchronization & Playhead Phase
To ensure sample-accurate synchronization with the DAW grid, the processor retrieves the host playhead position in `processBlock`. 

#### Fallback Free-Running Clock:
If the DAW transport is stopped, or the playhead is unavailable (e.g., in standalone mode), the processor transitions smoothly to a **Free-Running Sample Counter** so the gate pattern remains active.

```cpp
double bpm = 120.0;
double playheadSamples = 0.0;
bool isPlaying = false;

if (auto* playHead = getPlayHead()) {
    if (auto posInfo = playHead->getPositionInfo()) {
        bpm = posInfo->getBpm().value_or(120.0);
        playheadSamples = (double)posInfo->getTimeInSamples().value_or(0);
        isPlaying = posInfo->getIsPlaying();
    }
}

// Track position phase
if (isPlaying) {
    currentSamplePosition = playheadSamples;
} else {
    // Advance internal clock
    currentSamplePosition += numSamplesProcessed;
}
```

### 2.3 Step & Offset Calculations
Given the current sample position $s_{pos}$, host BPM, and selected rate (1/16, 1/8, 1/4):

1. **Step Duration in Seconds** ($T_{step}$):
   - **Rate 1/16**: $T_{step} = \frac{60.0}{BPM \cdot 4}$
   - **Rate 1/8**: $T_{step} = \frac{60.0}{BPM \cdot 2}$
   - **Rate 1/4**: $T_{step} = \frac{60.0}{BPM}$

2. **Step Duration in Samples** ($S_{step}$):
   $$S_{step} = T_{step} \cdot \text{sampleRate}$$

3. **Current Cycle & Step Index**:
   - Total samples in a 16-step loop: $S_{cycle} = S_{step} \cdot 16$
   - Cycle-relative sample position: $s_{cycle} = \text{fmod}(s_{pos}, S_{cycle})$
   - Current Step Index (0 to 15): 
     $$\text{step} = \lfloor \frac{s_{cycle}}{S_{step}} \rfloor$$
   - Offset within current step: 
     $$s_{offset} = s_{cycle} - (\text{step} \cdot S_{step})$$

### 2.4 Volume Contour Envelope Generator
For the current step, we retrieve its active state from the selected pattern bitmask:
$$\text{isActive} = (\text{patternMask} \& (1 \ll \text{step})) \neq 0$$

If `isActive` is true, the gate opens and closes based on the **Start**, **Hold**, and **End** envelope parameters:

- **Start Duration** ($D_{start}$ in samples): $\text{startMs} \cdot 0.001 \cdot \text{sampleRate}$ (clamped to max 50% of $S_{step}$).
- **Hold Width** ($W_{gate}$ in samples): $S_{step} \cdot (\text{holdPercent} \cdot 0.01)$.
- **End Duration** ($D_{end}$ in samples): $\text{endMs} \cdot 0.001 \cdot \text{sampleRate}$ (clamped to max 50% of $S_{step}$).

#### Envelope Phase Logic:
Within a single step period $t = s_{offset}$:
1. **Gate Closed (Inactive Step)**:
   $$\text{env}(t) = 0.0$$
2. **Gate Active Step**:
   - If $t < D_{start}$ (Attack phase):
     $$\text{env}(t) = \frac{t}{D_{start}}$$
   - If $D_{start} \le t < (W_{gate} - D_{end})$ (Hold phase):
     $$\text{env}(t) = 1.0$$
   - If $(W_{gate} - D_{end}) \le t < W_{gate}$ (Decay/Release phase):
     $$\text{env}(t) = 1.0 - \frac{t - (W_{gate} - D_{end})}{D_{end}}$$
   - If $t \ge W_{gate}$:
     $$\text{env}(t) = 0.0$$

#### Gating Attenuation & Mix:
Applying Depth ($D_{depth}$ from 0.0 to 1.0) and Dry/Wet Mix ($M_{mix}$ from 0.0 to 1.0):
- **Base Attenuation Level** (completely closed state): $A_{base} = 1.0 - D_{depth}$.
- **Gated Gain**: $G_{gated} = A_{base} + (D_{depth} \cdot \text{env}(t))$.
- **Final Sample Gain**:
  $$\text{gain}(t) = (1.0 - M_{mix}) + (M_{mix} \cdot G_{gated})$$

---

## 3. APVTS Parameter Configuration

We will add the following parameters in `PluginProcessor.cpp` under `createParameterLayout()`:

| Parameter ID | Name | Type | Range / Options | Default |
| :--- | :--- | :--- | :--- | :--- |
| `tg_active` | Gate Active | Bool | [Off, On] | Off |
| `tg_mix` | Gate Mix | Float | `0.0f` to `1.0f` | `1.0f` *(100% Gated)* |
| `tg_pattern` | Gate Pattern | Choice | Preset Pattern List (0 to 7) | `0` *(Straight 16th)* |
| `tg_rate` | Gate Rate | Choice | `["1/16", "1/8", "1/4"]` | `0` *(1/16)* |
| `tg_start` | Gate Start (Atk) | Float | `0.0f` to `100.0f` ms | `5.0f` ms |
| `tg_hold` | Gate Hold (Width) | Float | `10.0f` to `100.0f` % | `50.0f` % |
| `tg_end` | Gate End (Decay) | Float | `0.0f` to `200.0f` ms | `10.0f` ms |
| `tg_depth` | Gate Depth | Float | `0.0f` to `100.0f` % | `100.0f` % |

---

## 4. Web UI Component Design

The Trance Gate UI will be placed as a dedicated sub-panel inside Column 3, underneath the **Noise / Generator** panel, ensuring layout symmetry.

### 4.1 UI Layout Structure (HTML)
```html
<!-- TRANCE GATE PANEL -->
<div class="sub-panel" style="margin-top: 6px;">
    <div class="sub-panel-title">TRANCE GATE</div>
    
    <div style="display:flex; gap:10px; align-items:center; margin-bottom: 4px;">
        <div style="display:flex; gap:4px; align-items:center;">
            <input type="checkbox" id="tg_active">
            <div class="label">Active</div>
        </div>
        <select id="tg_pattern" style="width:110px;"></select>
        <select id="tg_rate" style="width:50px;">
            <option value="0">1/16</option>
            <option value="1">1/8</option>
            <option value="2">1/4</option>
        </select>
    </div>

    <!-- 16-STEP PREVIEW GRID -->
    <div class="tg-step-grid" id="tg_grid_container" style="display: grid; grid-template-columns: repeat(16, 1fr); gap: 2px; margin-bottom: 6px; padding: 2px;">
        <!-- Generating 16 steps programmatically -->
    </div>

    <!-- TG PARAMETERS IN 5-COLUMN GRID -->
    <div style="display: grid; grid-template-columns: repeat(5, 1fr); gap: 4px; text-align: center;">
        <!-- Mix, Depth, Start, Hold, End Knobs -->
    </div>
</div>
```

### 4.2 Styling & Theming (CSS)
All colors reference the dynamically injected CSS custom variables to respect the 60/30/10 rule.

```css
/* Step Grid LEDs */
.tg-step-led {
    height: 8px;
    background: var(--display-bg);
    border: 1px solid var(--panel-border);
    border-radius: 1.5px;
    box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.5);
    transition: background-color 0.15s ease, box-shadow 0.15s ease;
}

.tg-step-led.active {
    background: var(--primary);
    border-color: var(--primary);
    box-shadow: 0 0 4px var(--primary), inset 0 1px 0 rgba(255, 255, 255, 0.3);
}
```

### 4.3 JavaScript Controller (UI Sync)
```javascript
const tgPatterns = [0xAAAA, 0x5555, 0xEEEE, 0x9999, 0xF0F0, 0xD7D7, 0x8888, 0x8912];

function updateStepGrid(patternIndex) {
    const mask = tgPatterns[patternIndex] || 0;
    const container = document.getElementById('tg_grid_container');
    container.innerHTML = '';
    
    for (let step = 0; step < 16; ++step) {
        const led = document.createElement('div');
        led.className = 'tg-step-led';
        const isActive = (mask & (1 << step)) !== 0;
        if (isActive) {
            led.classList.add('active');
        }
        container.appendChild(led);
    }
}

// Bind to tg_pattern choice parameter updates
```

---

## 5. Implementation Roadmap

1. **DSP Integration**: Add `TranceGateProcessor.h` to the DSP source tree. Instance the processor in `PluginProcessor.h` and place it in the serial processing line inside `PluginProcessor::processBlock` after stage D (Mix) but before stage E (Final Gain).
2. **APVTS & Host Sync**: Register the parameters in `createParameterLayout()` and hook up playhead position fetching.
3. **HTML/JS Layout**: Add the sub-panel code to `index.html`, construct the 16-step visual LED grid, and bind the parameters to the bidirectional JS bridge.
4. **Compile & Verification**: Build and test the gated envelope across multiple rates (1/16, 1/8, 1/4) in a DAW to ensure sample-accurate tempo sync.
