'# Implementation Design: Sidechain Support (Internal & External)

**Location:** `doc/features_implementation/implementation_sidechain_support.md`
**Association:** `doc/features/feature_sidechain_support.md`

---

## 1. Overview
The sidechain system enables dynamic modulation of Mushin's DSP parameters (Drive, Filter Cutoff, Master Gain) using either an external audio signal from the DAW or an internally filtered version of the main input.

---

## 2. Bus Configuration
To support external sidechaining, the `MushinAudioProcessor` must be updated to handle multiple input buses.

### 2.1 Constructor Update
Modify the `BusesProperties` in the `MushinAudioProcessor` constructor:
```cpp
MushinAudioProcessor::MushinAudioProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                     .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)) // Secondary input
```

### 2.2 `isBusesLayoutSupported` Update
Ensure the sidechain bus is correctly validated:
```cpp
bool MushinAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Main output must be mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Main input must match main output
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    // Sidechain input can be mono or stereo (or disabled)
    auto scInput = layouts.getInputChannelSet(1);
    if (scInput != juce::AudioChannelSet::disabled() && 
        scInput != juce::AudioChannelSet::mono() && 
        scInput != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
```

---

## 3. DSP Architecture

### 3.1 `mushin::EnvelopeFollower`
A new utility class in `Source/dsp/EnvelopeFollower.h` to extract the amplitude envelope.

```cpp
namespace mushin {
class EnvelopeFollower {
public:
    void prepare(double sampleRate) {
        this->sampleRate = sampleRate;
    }

    void setParameters(float attackMs, float releaseMs, bool isRMS) {
        attack = std::exp(-1.0 / (sampleRate * attackMs * 0.001));
        release = std::exp(-1.0 / (sampleRate * releaseMs * 0.001));
        rmsMode = isRMS;
    }

    float processSample(float input) {
        float x = std::abs(input);
        if (rmsMode) x = x * x;

        if (x > envelope) envelope = attack * (envelope - x) + x;
        else              envelope = release * (envelope - x) + x;

        return rmsMode ? std::sqrt(envelope) : envelope;
    }

private:
    double sampleRate = 44100.0;
    float attack = 0.0f, release = 0.0f;
    float envelope = 0.0f;
    bool rmsMode = false;
};
}
```

### 3.2 Internal Sidechain Filter
A simple `juce::dsp::IIR::Filter` (Highpass/Lowpass) will be added to the `MushinAudioProcessor` to pre-process the internal sidechain signal before envelope detection.

---

## 4. Parameter Design (APVTS)

| Parameter ID | Name | Type | Range | Default |
|--------------|------|------|-------|---------|
| `sc_active` | SC Active | Bool | [Off, On] | Off |
| `sc_source` | SC Source | Choice | [Internal, External] | Internal |
| `sc_threshold` | SC Threshold | Float | [-60dB, 0dB] | -24dB |
| `sc_attack` | SC Attack | Float | [0.1ms, 500ms] | 10ms |
| `sc_release` | SC Release | Float | [1ms, 2000ms] | 100ms |
| `sc_mode` | SC Mode | Choice | [Peak, RMS] | Peak |
| `sc_amount` | SC Amount | Float | [-100%, 100%] | 0% |
| `sc_target` | SC Target | Choice | [Drive, Cutoff, Gain] | Gain |
| `sc_hp_freq` | SC HPF | Float | [20Hz, 2000Hz] | 20Hz |
| `sc_lp_freq` | SC LPF | Float | [500Hz, 20000Hz] | 20000Hz |

---

## 5. Integration in `processBlock`

The sidechain signal processing will occur within the sample-by-sample loop:

1. **Extract Sidechain Sample:**
   - If `sc_source == External`: Read from `getBusBuffer (true, 1)`.
   - If `sc_source == Internal`: Read from main input buffer (pre-distortion).
2. **Filter (Optional):** Apply SC HPF/LPF to the sidechain sample.
3. **Envelope Follow:** Pass sample through `mushin::EnvelopeFollower`.
4. **Modulate:** 
   - Calculate modulation value: `Mod = (Envelope > Threshold) ? (Envelope - Threshold) * Amount : 0`.
   - Apply `Mod` to the selected `sc_target`.

---

## 6. UI Integration

- **New Component:** `SidechainPanel` in the React/Web frontend.
- **Visualizer:** A small gain reduction/modulation meter showing the envelope level relative to the threshold.
- **Bridge Updates:** Register the new parameters in `MushinAudioProcessor::createParameterLayout()` and ensure they are synced to the frontend.

---

## 7. Validation Strategy

1. **Bus Visibility:** Verify that "Mushin Sidechain" appears as an input option in Ableton Live, Logic Pro, and Reaper.
2. **Phase Correlation:** Ensure internal sidechaining doesn't introduce unwanted phase issues (though it's a modulation source, so phase matters less than timing).
3. **Latency:** Verify zero-latency operation for the sidechain path.
4. **UI Responsiveness:** Ensure the SC meter in the Web UI is fluid and accurate.
