# Mushin – A Modern Audio Plugin

Mushin is a free, open‑source audio plugin that blends classic distortion and filtering with modern side‑chain, limiter, and modulation tools.  
It ships as a VST3/Standalone application for Windows, macOS and Linux (via JUCE). The UI is built in HTML/CSS/JS and rendered inside a WebView2 component on Windows or the native web view on other platforms.

> **Why Mushin?**  
> *Mushin* means “to practice” in Japanese.  The plugin encourages experimentation: tweak knobs, mix side‑chain, apply gating patterns, and save presets – all from a single, lightweight interface.

---

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Supported Hosts & Platforms](#supported-hosts--platforms)
- [Using the Plugin](#using-the-plugin)
  - [Parameters](#parameters)
  - [Side‑Chain / Limiter](#side-chain--limiter)
  - [Trance Gate](#trance-gate)
  - [Noise Generator](#noise-generator)
  - [Preset Management](#preset-management)
- [Keyboard Shortcuts](#keyboard-shortcuts)
- [Development](#development)
  - [Building from Source](#building-from-source)
  - [Testing](#testing)
- [License](#license)

---

## Features

| Feature | Description |
|---------|-------------|
| **Distortion & Filtering** | Two independent filter chains (LP/HP/BP/Notch) with selectable types and resonance. Distortion modes: Clean, Vintage, Acid, Digital. |
| **Side‑Chain / Limiter** | Real‑time side‑chain detection, peak meter, limiter gain reduction display. Adjustable attack/release, mix, drive, ceiling. |
| **Trance Gate** | 16‑step gating patterns (Straight 16th, Offbeat 16th, Classic 1/2, Four‑On‑Floor, Galop, Space Gate, Euclidean 5). Custom step width, attack, decay, hold. |
| **Noise Generator** | White/Pink/Sine/Triangle/Sawtooth/Square noise with frequency, level, routing (Pre‑Dist / Pre‑Filter / Post‑Filter). FM modulation support. |
| **Modulation LFOs** | Two LFOs controlling any parameter via a 16‑step matrix. Waveforms: Sine, Tri, Saw, Square, Random. |
| **Preset Manager** | Save/load/delete presets in the plugin’s own folder. Searchable list with keyboard navigation. |
| **Theme System** | Eight UI themes (Industrial, Synthwave, Acid, Firepits, Ocean Deep, Ice World, Dark Hellish). Theme persists across sessions. |
| **WebView2 Integration** | Full HTML/CSS/JS UI with custom bridge to C++ for parameter sync and preset handling. |

---

## Installation

1. Download the latest release from the [Releases](https://github.com/yourrepo/mushin/releases) page.
2. Extract the ZIP file.
3. Copy the `Mushin.vst3` (and optionally the standalone executable) to your plugin folder:
   - **Windows**: `%ProgramFiles%\Steinberg\VSTPlugins`
   - **macOS**: `/Library/Audio/Plug‑Ins/VST3`
   - **Linux**: `~/.vst3`

4. Open your DAW and scan for new plugins.

---

## Supported Hosts & Platforms

| Platform | Host | Notes |
|----------|------|-------|
| Windows 10+ | Any VST3 host (Ableton, FL Studio, Reaper, etc.) | Requires WebView2 runtime (included in the installer). |
| macOS 12+ | Any VST3 host | Uses native WebKit view. |
| Linux | Reaper, Ardour | Tested on Ubuntu 22.04; may need `libwebkit2gtk-4.0-dev`. |

---

## Using the Plugin

### Parameters

All parameters are exposed via the JUCE `AudioProcessorValueTreeState` and can be automated in your DAW.  
The UI shows a real‑time value next to each knob/slider.

| Category | Parameter | Default |
|----------|-----------|---------|
| **Distortion** | Drive | 1.0 |
| | Mix | 50% |
| | Threshold | -12 dB |
| | Exhaustion | OFF |
| | Autogain | OFF |
| **Filter A / B** | Cutoff | 20000 Hz |
| | Resonance | 0.5 |
| | Type | Clean |
| | Mode | LP |
| **LFO1 / LFO2** | Frequency | 4 Hz |
| | Waveform | Sine |
| | Target (16 params) | None |
| **Side‑Chain** | Active | OFF |
| | Mix | 50% |
| | Drive | 0 dB |
| | Attack/Release | 10 / 100 ms |
| **Limiter** | Active | OFF |
| | Mix | 50% |
| | Drive | 24 dB |
| | Ceiling | -12 dB |
| | Release | 200 ms |
| **Trance Gate** | Active | OFF |
| | Pattern | Straight 16th |
| | Rate | 1/8 |
| | Depth | 100% |
| | Attack / Decay / Hold | 10 / 20 / 50 ms |
| **Noise** | Active | OFF |
| | Type | White Noise |
| | Frequency | 440 Hz |
| | Level | -30 dB |
| | Routing | Pre‑Dist |

> *Tip:* Use the “Reset to Default” double‑click on any knob to restore its factory value.

### Side‑Chain / Limiter

- **Side‑Chain**: Enable to trigger the limiter based on an external input (e.g., kick drum).  
  The peak meter shows the detected level; the limiter gain reduction meter displays how much attenuation is applied.
- **Limiter**: When active, it clamps peaks above the ceiling. Adjust `Drive` for more aggressive limiting.

### Trance Gate

The gate uses a 16‑step pattern. Each step can be turned on/off via the pattern mask.  
Use the “Depth” slider to blend the gated signal with the original.  
You can also set custom attack/decay times per step.

### Noise Generator

Add subtle texture or rhythmic noise. The FM modulation knob lets you modulate the frequency with a low‑frequency oscillator.

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **Ctrl+S** (Cmd+S on macOS) | Save current preset |
| **Ctrl+O** (Cmd+O) | Load preset dialog |
| **Ctrl+D** (Cmd+D) | Delete selected preset |
| **Ctrl+F** (Cmd+F) | Focus preset search box |
| **Arrow keys** | Navigate preset list |
| **Enter** | Load highlighted preset |

---

## Development

### Building from Source

1. Install the latest JUCE 8.0.4 and CMake ≥ 3.20.
2. Clone the repo:
   ```bash
   git clone https://github.com/yourrepo/mushin.git
   cd mushin
   ```
3. Create a build directory and run CMake:
   ```bash
   mkdir build && cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```
4. Open the generated solution in Visual Studio, build `Mushin` (Release/Debug).

> **Windows**: The WebView2 loader DLL is automatically fetched by JUCE’s FetchContent.

### Testing

- Run the standalone executable to test UI and preset handling.
- Use Reaper or another host for automated parameter changes.
- Unit tests are not included; manual testing is recommended.

---

## License

Mushin is released under the **Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International(CC BY-NC-ND 4.0)**.  
See `LICENSE` for details.

--- 

*Happy mixing!*
