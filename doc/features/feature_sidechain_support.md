# Feature: Sidechain Support (Internal & External)

## Overview
Enable the processor to be modulated by a secondary signal. 
- **External Sidechain:** Traditional ducking/gating or modulation triggered by another track in the DAW.
- **Internal Sidechain:** Self-modulation where a filtered/processed version of the input signal controls the main DSP (e.g., frequency-dependent saturation).

## Architecture

### 1. Audio Bus Configuration
- **Main Input:** Bus 0 (Stereo).
- **Sidechain Input:** Bus 1 (Stereo/Mono).
- **MushinAudioProcessor** must override `isBusesLayoutSupported` to permit an auxiliary input bus.
- Update `BusesProperties` in the constructor to include `.withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)`.

### 2. Sidechain DSP Logic
- **Envelope Follower:** A dedicated class to extract the amplitude envelope of the sidechain signal.
    - Parameters: Attack, Release, Mode (Peak/RMS).
- **Routing Matrix:**
    - `SC_SOURCE`: None, Internal (Pre-Distortion), External.
    - `SC_DESTINATION`: Drive Amount, Filter Cutoff, or Master Gain.
- **Internal Sidechain Path:** Input signal -> Highpass/Lowpass Filter -> Envelope Follower -> Modulation Target.

### 3. Parameters (APVTS)
- `sc_active` (Bool): Toggle sidechain modulation.
- `sc_source` (Choice): [Internal, External].
- `sc_threshold` (Float): Threshold for modulation trigger.
- `sc_attack` / `sc_release` (Float): Timing for the envelope follower.
- `sc_amount` (Float): Bipolar amount (-100% to 100%) to control how much the envelope affects the target.

### 4. UI Implementation
- **Sidechain Panel:** A dedicated expandable section or tab in the Web UI.
- **Visual Feedback:** A small meter showing the sidechain input level and the resulting gain reduction/modulation amount.
- **Source Toggle:** Buttons to switch between "INT" and "EXT".

## Implementation Steps

1. **Bus Management:**
    - Update `MushinAudioProcessor` constructor and `isBusesLayoutSupported`.
    - Modify `processBlock` to retrieve the sidechain buffer using `getBusBuffer (true, 1)`.
2. **DSP Development:**
    - Implement a simple `EnvelopeFollower` class in `Source/dsp`.
    - Integrate the modulation logic into the main `processBlock` loop (applying the envelope value to the `LinearSmoothedValue` targets).
3. **UI Integration:**
    - Add sidechain controls to `index.html`.
    - Update the bridge to handle new sidechain parameters.
4. **Validation:**
    - Test in DAWs (Ableton, Logic, Reaper) to ensure the sidechain bus is correctly exposed.
    - Verify that internal sidechaining doesn't create feedback loops or stability issues.
