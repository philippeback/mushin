# Implementation Design: Noise Oscillator & Test Signal Generator

**Location:** `doc/features_implementation/implementation_noise_oscillator.md`
**Association:** `doc/features/feature_noise_oscillator.md`

---

## 1. Overview
The Noise Oscillator / Test Signal Generator provides Mushin with a modular, high-quality audio signal source. It serves two main purposes:
1. **Audio Synthesis (Noise Injector)**: Dynamically adding rich noise textures (white or pink noise) or test oscillator tones directly into the processing pipeline.
2. **Standalone Test Drive**: Providing a self-contained sound source to easily test and demonstrate the plugin's DSP effects (distortion, filtering, sidechaining) when no active external audio input is present in the host DAW or Standalone wrapper.

---

## 2. DSP Engine: `mushin::NoiseOscillator`
A new C++ class, `mushin::NoiseOscillator` (located in `Source/dsp/NoiseOscillator.h`), will generate the synthesized signal. It supports six generator types:

| Type | Name | Description | Synthesis Details |
|------|------|-------------|-------------------|
| `0` | White Noise | Uniform full-spectrum energy noise | Pseudo-random float generated sample-by-sample. |
| `1` | Pink Noise | $-3$ dB/octave energy roll-off noise | Voss-McCartney method filtered with a Paul Kellet 3-decade pinking filter. |
| `2` | Sine Tone | Fixed or variable pure sinusoid | Phase accumulator mapped via `std::sin`. |
| `3` | Triangle Wave | Classic linear ramp up/down wave | Phase accumulator mapped to absolute triangle function. |
| `4` | Sawtooth Wave | Linear rising ramp wave | Phase accumulator scaled from $-1$ to $+1$. |
| `5` | Square Wave | Bipolar pulse wave | Phase accumulator thresholded at $0.5$. |

### 2.1 Pink Noise Spectral Pinking Filter
```cpp
// Pink noise generator (Paul Kellet's refined 3-pole pinking filter approximation)
class PinkNoiseSource {
public:
    float nextSample() {
        float white = ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f; // Normalize output scale to match white noise volume
    }
private:
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f, b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;
};
```

---

## 3. Parameter Design (APVTS)
To integrate the Noise Oscillator with host automation, preset saving/loading, and the Web UI, we will register six new parameters in `MushinAudioProcessor::createParameterLayout()`:

| Parameter ID | Name | Type | Range | Default | Description |
|--------------|------|------|-------|---------|-------------|
| `noise_active` | Noise Active | Bool | [Off, On] | Off | Global On/Off toggle for the generator. |
| `noise_type` | Noise Type | Choice | [White, Pink, Sine, Triangle, Saw, Square] | White | Waveform/Noise profile selection. |
| `noise_freq` | Noise Freq | Float | [20.0 Hz, 20000.0 Hz] (Skew: 0.3) | 440.0 Hz | Oscillator frequency for Tone types. |
| `noise_level` | Noise Level | Float | [0.0 (Silence), 1.0 (0dB)] | 0.1 | Master injection volume. |
| `noise_routing` | Noise Routing | Choice | [Pre-Dist, Pre-Filter, Post-Filter] | Pre-Dist | Stage injection destination. |
| `noise_fm_mod` | Filter FM Mod | Float | [0.0 (0%), 1.0 (100%)] | 0.0 | Amount of filter cutoff FM modulation. |

---

## 4. Integration into DSP Pipeline (`processBlock`)
The Noise Oscillator must support injecting its signal at three key stages in `processBlock`.

```cpp
void MushinAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // ... setup and sidechain ...
    int numSamples = buffer.getNumSamples();
    
    // Sample-by-sample processing loop
    for (int s = 0; s < numSamples; ++s)
    {
        // 1. Synthesize current sample of noise/oscillator
        float genSample = 0.0f;
        if (noiseActiveParam->load() > 0.5f) {
            genSample = noiseOscillator.nextSample() * noiseLevelParam->load();
        }

        // Apply dynamic FM Modulation to filter cutoff frequencies if active
        float fmAmount = noiseFmModParam->load();
        float currentFmMod = genSample * fmAmount; // Bipolar modulation offset

        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float drySample = dryBuffer.getSample(ch, s);
            float wetSample = drySample;

            // STAGE A: Distortion / Waveshaper
            if (noiseActiveParam->load() > 0.5f && noiseRoutingParam->load() == 0) { // Pre-Dist
                wetSample += genSample;
            }

            float currentDrive = driveParam->load();
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Drive) {
                currentDrive += (scMod * 20.0f);
            }
            waveshaper.setDrive(std::clamp(currentDrive, 1.0f, 50.0f));
            wetSample = waveshaper.processSample(ch, wetSample);

            // STAGE B: Dual Filtering
            if (noiseActiveParam->load() > 0.5f && noiseRoutingParam->load() == 1) { // Pre-Filter
                wetSample += genSample;
            }

            // Update Cutoff base values (accounting for Sidechain)
            float baseCutoffA = filterACutoffParam->load();
            float baseCutoffB = filterBCutoffParam->load();
            if (sidechainProcessor.isActive() && sidechainProcessor.getTarget() == mushin::SidechainProcessor::Target::Cutoff) {
                float cutoffOffset = std::pow(2.0f, scMod * 2.0f);
                baseCutoffA *= cutoffOffset;
                baseCutoffB *= cutoffOffset;
            }

            // Apply LFO matrix offsets and add high-frequency FM cutoff modulation!
            float modOffsets[6] = { 0.0f };
            if (ch == 0) {
                float lfo1Val = dualFilterSystem.getLFO1().getNextSample();
                float lfo2Val = dualFilterSystem.getLFO2().getNextSample();
                
                // standard LFO offsets
                for (int t = 0; t < 6; ++t) {
                    modOffsets[t] = (lfo1Val * modMatrix[0][t]) + (lfo2Val * modMatrix[1][t]);
                }
                
                // Add FM modulation offset from the generator to Cutoffs
                modOffsets[0] += currentFmMod; // Target 1: Filter A Cutoff
                modOffsets[3] += currentFmMod; // Target 4: Filter B Cutoff

                dualFilterSystem.applyModulation(modOffsets);
            }

            wetSample = dualFilterSystem.processSample(ch, wetSample);

            // STAGE C: Post-Filter Injection
            if (noiseActiveParam->load() > 0.5f && noiseRoutingParam->load() == 2) { // Post-Filter
                wetSample += genSample;
            }

            // STAGE D: Dry/Wet Mix & Master Gain
            float mixedSample = (drySample * (1.0f - mixVal)) + (wetSample * mixVal);
            // ... Final gain and write-back ...
        }
    }
}
```

---

## 5. UI Integration
We will add a new premium retro panel to the Web UI inside Column 3.

### 5.1 HTML Layout addition:
```html
<div class="sub-panel">
    <div class="sub-panel-title">NOISE / GENERATOR</div>
    <div style="display:flex; gap:10px; align-items:center; margin-bottom: 4px;">
        <div style="display:flex; gap:4px; align-items:center;">
            <input type="checkbox" id="noise_active">
            <div class="label">Active</div>
        </div>
        <select id="noise_routing" style="width:75px;">
            <option value="0">Pre-Dist</option>
            <option value="1">Pre-Filter</option>
            <option value="2">Post-Filter</option>
        </select>
    </div>
    <div style="display:flex; gap:10px; align-items:center; margin-bottom: 6px;">
        <div class="label" style="flex:1;">Type</div>
        <select id="noise_type" style="width:115px;">
            <option value="0">White Noise</option>
            <option value="1">Pink Noise</option>
            <option value="2">Sine Tone</option>
            <option value="3">Triangle Wave</option>
            <option value="4">Sawtooth Wave</option>
            <option value="5">Square Wave</option>
        </select>
    </div>
    <div class="matrix-grid">
        <div style="text-align: center;">
            <div class="label">Freq</div>
            <div class="mushin-knob mushin-knob-small" id="knob-noise_freq" data-param="noise_freq" data-default="0.25" tabindex="0">
                <!-- SVG structure mirroring small knobs -->
            </div>
            <div class="value" id="noise_freq-val">440Hz</div>
        </div>
        <div style="text-align: center;">
            <div class="label">Level</div>
            <div class="mushin-knob mushin-knob-small" id="knob-noise_level" data-param="noise_level" data-default="0.1" tabindex="0">
                <!-- SVG structure mirroring small knobs -->
            </div>
            <div class="value" id="noise_level-val">-20dB</div>
        </div>
    </div>
    <div style="text-align: center; margin-top: 4px;">
        <div class="label">Filter FM Mod</div>
        <div style="display: flex; align-items: center; justify-content: center; gap: 8px;">
            <div class="mushin-knob mushin-knob-small" id="knob-noise_fm_mod" data-param="noise_fm_mod" data-default="0.0" tabindex="0">
                <!-- SVG structure mirroring small knobs -->
            </div>
            <div class="value" id="noise_fm_mod-val">0%</div>
        </div>
    </div>
</div>
```

---

## 6. Verification Plan

### 6.1 Automated Compilation
- Compile all targets (including standalone executable and VST3 format) to ensure zero compilation or linking issues:
  ```powershell
  cmake --build build2 --config Debug
  ```

### 6.2 Manual Functional Verification
1. **Generators**: Activate the panel, set routing to Post-Filter, and select each generator type. Verify that White Noise and Pink Noise sound clean and distinct. Verify that Sine, Triangle, Saw, and Square tones produce stable, clean outputs.
2. **Frequency Control**: Sweep the `Freq` knob and verify it smoothly adjusts the pitch of tone generators from $20$ Hz to $20$ kHz.
3. **Routing Stages**: 
   - Pre-Dist: Waveshaping should apply saturation/distortion to the noise/oscillator.
   - Pre-Filter: Saturation is bypassed, but filtering affects the tone.
   - Post-Filter: Tone passes completely dry and dry/wet controls do not isolate it.
4. **Filter FM Mod**: Increase `Filter FM Mod` and verify that the filter cutoff is dynamically modulated by the generator's waveform.
