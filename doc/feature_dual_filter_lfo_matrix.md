# Feature: Dual Filter System with LFO & Modulation Matrix

## Overview
Elevate the sound shaping capabilities by introducing a sophisticated dual filter section. Inspired by high-end synthesizers like Air Hybrid 3, this system provides flexible routing, diverse filter models, and a robust modulation matrix.

## Architecture

### 1. Dual Filter Section
- **Filters:** Filter A and Filter B.
- **Routing Modes:**
    - **Serial:** Input -> Filter A -> Filter B -> Output.
    - **Parallel:** Input splits to Filter A and Filter B; outputs are summed.
- **Filter Types:**
    - Clean (TPT), Vintage (Ladder), Acid (Diode), and Digital (State Variable).
    - Modes: Lowpass (6/12/24 dB), Highpass, Bandpass, Notch.
- **Smart Features (Grit & Saturation):**
    - Each filter has a dedicated **Pre-Filter Drive** and **Post-Filter Grit** control to add harmonic character before or after the resonance peak.

### 2. Modulation Sources (LFO)
- **Dual LFOs:** LFO 1 and LFO 2.
- **Waveforms:** Sine, Triangle, Saw, Square, Random (S&H).
- **Sync:** Beat-sync to host tempo or free-running (Hz).
- **Retrigger:** Option to reset phase on MIDI note-on or run continuously.

### 3. Modulation Matrix (The "Brain")
A bipolar matrix that connects sources to targets with specific amounts.
- **Sources:** LFO 1, LFO 2, Envelope Follower (from Sidechain), MIDI Velocity, Mod Wheel.
- **Targets:**
    - Filter A/B Cutoff.
    - Filter A/B Resonance.
    - Filter A/B Grit/Drive.
    - Master Mix/Gain.
- **Scaling:** Modulation depth can be controlled by a secondary source (e.g., LFO 1 amount controlled by the Mod Wheel).

## Implementation Steps

### 1. DSP Engine Enhancements
- **Filter Classes:** Refactor the existing single filter to a `mushin::Filter` class that supports multiple types and internal saturation.
- **Routing Logic:** Implement a `FilterContainer` that manages the Serial/Parallel logic.
- **LFO Class:** Create a sample-accurate `mushin::LFO` class using `juce::dsp::Oscillator`.

### 2. Modulation System
- **Modulator Interface:** Define a standard way for LFOs and Envelopes to provide values.
- **The Matrix Loop:** In `processBlock`, calculate the modulation offsets before updating the `LinearSmoothedValue` targets of the filters.
    - *Formula:* `FinalTarget = BaseValue + (ModSource * ModAmount)`.

### 3. UI Implementation
- **Filter Tabs:** Toggle between Filter A and Filter B settings.
- **Routing Visualizer:** A small diagram showing the signal flow (Serial vs Parallel).
- **Matrix Grid:** A dedicated view to manage assignments (Source -> Target -> Amount).
- **LFO Visualizers:** Real-time waveform previews.

## Validation
- **Phase Alignment:** Ensure parallel routing doesn't cause unwanted cancellations.
- **CPU Optimization:** Use SIMD for the dual-filter processing loop.
- **Modulation Smoothness:** Verify that high-frequency modulation (e.g., LFO at 50Hz) doesn't cause "zipper noise" (utilize per-sample smoothing).
