# Mushin – Web‑Based Audio Effect Plugin

**Mushin** is a modern audio‑effect plugin that blends classic analog‑style filtering with contemporary modulation techniques, all controlled through an HTML/CSS/JS interface rendered inside JUCE’s `WebBrowserComponent`.  
The project is built with CMake and pulls the JUCE framework automatically via `FetchContent`.

---

## Why Mushin?

* **Intuitive UI** – The editor is a web page that feels like a native app. Themes, presets, and real‑time visualisations are all handled in JavaScript.
* **Cross‑platform** – Works as a VST3 on Windows/macOS/Linux and also as a standalone application.
* **Extensible DSP** – Built‑in Trance Gate, four‑band filter, side‑chain limiter, and waveshaper. Adding new modules is straightforward thanks to the clean separation between processor logic and UI bridge.
* **Persistent settings** – Themes are stored in a tiny INI file under your application‑data folder, so your look‑and‑feel stays consistent across sessions.

---

## Features

| Feature | What it does |
|---------|--------------|
| **Trance Gate** | 16‑step gate with attack/decay/hold envelopes and preset patterns (Straight 16th, Offbeat 16th, Classic 1…). |
| **Four‑Band Filter** | Clean, Vintage (ladder), Acid, Digital – each band can be lowpass/highpass/bandpass/notch. |
| **Sidechain & Limiter** | High‑pass side‑chain detection + hardware‑style limiter meter. |
| **Waveshaper** | Simple tanh distortion that can be extended to more complex curves. |
| **Preset Management** | Save, load, delete, and list presets directly from the UI or via the C++ API. |
| **Theme Switching** | Six custom themes (Industrial, Synthwave, Acid, Firepits, Ocean Deep, Ice World) that are persisted in a tiny INI file. |

---

## Getting Started

### Prerequisites

* **Windows** – Visual Studio 2022 (or newer) + CMake 3.20+  
* **macOS** – Xcode 13+ (CMake 3.20+)  
* **Linux** – Any recent GCC/Clang with CMake 3.20+

> The project pulls JUCE automatically; no manual installation is required.

### Build

