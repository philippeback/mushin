# Feature Implementation: Readability, 60/30/10 Theme Harmonization, & Silent Sidechain Test Routing

**Location:** `doc/features_implementation/implementation_readability_skinning_sidechain_improvements.md`  
**Date:** May 2026  
**Session Scope:** High-contrast legibility, UI scaling, color design guidelines, and silent test-signal routing for sidechain modulation.

---

## 1. Overview & Context

This document provides a comprehensive design and engineering record of the usability, aesthetic, and functional upgrades implemented in the Mushin synthesizer. The session focused on three main pillars:
1. **Readability & Usability**: Substantially enlarging text labels, panel headers, knobs, sliders, and mechanical toggles to ensure high-density screen readability.
2. **Dynamic 60/30/10 Color Harmonization**: Standardizing all 7 built-in themes to strictly adhere to the professional 60/30/10 color rule.
3. **Internal Silent Sidechain Routing**: Creating a `"Test Signal"` source for the Sidechain, enabling sidechain testing internally using the Noise/Generator panel without audio bleeding into the master output.

---

## 2. UI Readability & Control Sizing Upgrades

### 2.1 Problem Statement
On high-DPI monitors, the previous text size and control elements were too small, leading to squinting and reduced control precision. Panel header labels were hard to locate, and mechanical switches lacked spatial significance.

### 2.2 Sizing Transformations & CSS Classes
We modified the UI core stylesheet (`Source/Web/index.html` or compiled assets) to apply a consistent scaling factor:
- **Panel Title Headers (`.sub-panel-title`)**: Enlarged to **2x** their original size, transforming from compact uppercase text into prominent, readable category markers.
- **Control Labels (`.label`)**: Enlarged by **1.5x–2x** to ensure quick identification of parameter controls.
- **Knobs & Sliders**: Scaled up to maximize contact surface and make the metallic reflections/bezel detailing visually premium.
- **Top-Level Controls (e.g., DRIVE, FREQ)**: Given special sizing treatments to represent their primary signal flow importance.
- **Routing & Mod-Matrix Columns**: Given the same resizing treatment, standardizing Mod-Matrix source labels and control sliders to match the main panel columns.

---

## 3. Dynamic 60/30/10 Color Harmonization

### 3.1 The 60/30/10 Design Rule
To achieve professional visual depth and elite aesthetics, each theme was refactored under a strict color distribution model:
- **60% Dominant (Canvas / Surfaces)**: `--bg-hardware` represents the main chassis frame. It utilizes premium dark basalt/obsidian shades with subtle thematic shifts (violet, moss, charcoal, deep navy, icy gray) to ground the visual hierarchy.
- **30% Secondary (Card Slots, Text, Borders)**: 
  - `--display-bg` provides slightly lighter card containers establishing physical boundaries.
  - `--text-main` provides warm/cool tinted whites ensuring high-contrast legibility against dark surfaces.
  - `--secondary` provides strong secondary colors for panel titles.
  - `--panel-border` defines machined boundaries.
- **10% Accent (Highlights & Actions)**:
  - `--primary` provides bright glowing neons reserved exclusively for active sliders, knobs, active toggles, and buttons.
  - `--sc-yellow` provides glowing yellow-orange peak signal warnings for meter highlights.

### 3.2 The Harmonized Theme Matrix
All themes are served dynamically as a virtual `skin.css` stylesheet by the resource provider in [PluginEditor.cpp](file:///C:/Dev/github/philippeback/mushin/Source/PluginEditor.cpp). The updated variable values are detailed below:

| Theme Name | `--bg-hardware` (60%) | `--display-bg` / `--text-main` / `--secondary` / `--panel-border` (30%) | `--primary` / `--sc-yellow` / `--sc-input` (10%) |
| :--- | :--- | :--- | :--- |
| **Synthwave** | `#14091a` *(Deep Obsidian Indigo)* | `--display-bg`: `#200f29`<br>`--text-main`: `#ebd6f5`<br>`--secondary`: `#00ffff` *(Neon Cyan)*<br>`--panel-border`: `#461c5c` | `--primary`: `#ff00aa` *(Neon Hot Pink)*<br>`--sc-yellow`: `#ffff00`<br>`--sc-input`: `#00ffcc` |
| **Acid** | `#091209` *(Toxic Obsidian Green)* | `--display-bg`: `#122412`<br>`--text-main`: `#e2ebd2`<br>`--secondary`: `#39ff14` *(Neon Lime Green)*<br>`--panel-border`: `#284d28` | `--primary`: `#bfff00` *(Acid Lime)*<br>`--sc-yellow`: `#dfff4f`<br>`--sc-input`: `#00ff3c` |
| **Firepits** | `#120906` *(Obsidian Charcoal/Ember)* | `--display-bg`: `#22120b`<br>`--text-main`: `#f5e6de`<br>`--secondary`: `#ff6600` *(Volcanic Orange)*<br>`--panel-border`: `#4d2010` | `--primary`: `#ff3c00` *(Searing Lava Red)*<br>`--sc-yellow`: `#ffcc00`<br>`--sc-input`: `#ff3333` |
| **Ocean Deep** | `#060b1a` *(Deep Abyss Maritime Navy)* | `--display-bg`: `#0f1830`<br>`--text-main`: `#e3f0f5`<br>`--secondary`: `#00bfff` *(Bioluminescent Blue)*<br>`--panel-border`: `#1d305c` | `--primary`: `#00f3ff` *(Neon Aqua/Cyan)*<br>`--sc-yellow`: `#ccff33`<br>`--sc-input`: `#008080` |
| **Ice World** | `#0f151c` *(Deep Frosted Slate Blue)* | `--display-bg`: `#1a2430`<br>`--text-main`: `#ffffff`<br>`--secondary`: `#a5c1eb` *(Glacier Blue)*<br>`--panel-border`: `#384c61` | `--primary`: `#66e0ff` *(Aurora Ice Blue)*<br>`--sc-yellow`: `#ffff66`<br>`--sc-input`: `#e0ffff` |
| **Dark Hellish** | `#0e0606` *(Obsidian Hellfire Black)* | `--display-bg`: `#180a0a`<br>`--text-main`: `#e6cfcf`<br>`--secondary`: `#ff4d4d` *(Volcanic Coral)*<br>`--panel-border`: `#301414` | `--primary`: `#ff2200` *(Neon Hellfire Orange-Red)*<br>`--sc-yellow`: `#ffcc00`<br>`--sc-input`: `#990000` |
| **Industrial** | `#111111` *(Carbon Matte Black)* | `--display-bg`: `#1c1c1c`<br>`--text-main`: `#f0f0f0`<br>`--secondary`: `#ff9f00` *(Safety Amber Gold)*<br>`--panel-border`: `#333333` | `--primary`: `#00bfff` *(Electric Sky Blue)*<br>`--sc-yellow`: `#ffcc00`<br>`--sc-input`: `#00ff66` |

---

## 4. Silent Test Signal Sidechain Routing

### 4.1 Feature Description
The Sidechain modulator has been equipped with a third input source: `"Test Signal"`. This allows the user to route the synthesizer's built-in Noise / Generator panel audio directly into the sidechain envelope detector.

### 4.2 Silent Testing Architecture
To prevent loud test tones or white noise signals from bleeding into the master audio output during sidechain testing, we implemented a decoupled generation model in `PluginProcessor.cpp`:

1. **Decoupled Sample Calculation**: The Noise/Generator DSP (`noiseOscillator`) processes and generates `genSample` if the Noise Generator's **Active** switch is turned on **OR** if the Sidechain is active and set to the `"Test Signal"` source.
2. **Coupled Output Routing**: 
   - `genSample` is always available to drive the sidechain input buffer seamlessly when sidechain source = `TestSignal`.
   - However, `genSample` is only mixed into the master signal path (`wetSample`) at Stage A, B, or C if `noiseActive` is explicitly enabled.

This guarantees a premium quality-of-life workflow where you can compress or filter-duck the synthesizer silently using the internal generator without hearing a raw sine sweep or white noise hiss.

### 4.3 Code Implementation Details

#### Enum Definition (`SidechainProcessor.h`)
```cpp
enum class Source {
    Internal,
    External,
    TestSignal
};
```

#### APVTS Parameter Choice (`PluginProcessor.cpp`)
```cpp
params.push_back (std::make_unique<juce::AudioParameterChoice> (
    juce::ParameterID { "sc_source", 1 }, "SC Source", juce::StringArray {"Internal", "External", "Test Signal"}, 0));
```

#### Unified Process Loop Routing (`PluginProcessor.cpp`)
```cpp
// 1. Determine if generator sample is required
bool noiseActive = (noiseActiveParam != nullptr && noiseActiveParam->load() > 0.5f);
bool testSignalForSidechain = (sidechainProcessor.isActive() && 
                               sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::TestSignal);

if (noiseActive || testSignalForSidechain) {
    noiseOscillator.setType((int)noiseTypeParam->load());
    noiseOscillator.setFrequency(noiseFreqParam->load());
    genSample = noiseOscillator.nextSample() * noiseLevelParam->load();
}

// ...

// 2. Select input source for Sidechain Processor
if (sidechainProcessor.isActive()) {
    float scInputSample = 0.0f;
    if (sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::External && scBusEnabled) {
        scInputSample = getStereoMixedToMonoSample(sidechainBuffer, s);
    } else if (sidechainProcessor.getSource() == mushin::SidechainProcessor::Source::TestSignal) {
        scInputSample = genSample; // Silently routed generator sample
    } else {
        scInputSample = getStereoMixedToMonoSample(dryBuffer, s); // Internal dry main input
    }
    scMod = sidechainProcessor.processSample(scInputSample);
}
```

---

## 5. Verification & Version Control Summary

### 5.1 Verification Commands
The backend C++ code and embedded HTML/JS files were compiled and verified for build stability using the MSBuild generator in powershell:
```powershell
cmake --build build2 --config Debug --target Mushin_Standalone
```

### 5.2 Git Commits in this Session
The work has been successfully logged and committed to the Git repository under the `feature/fontsize` branch:
1. `colors moved to 60/30/10 design rule` — Standardized Synthwave, Acid, Firepits, Ocean Deep, Ice World, Dark Hellish, and Industrial themes.
2. `implement silent test signal routing to sidechain` — Added `"Test Signal"` source choice, updated processing loop, and added Web UI dropdown options.
