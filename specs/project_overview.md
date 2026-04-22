# Mushin Project Specification

## Project Overview
**Mushin** is a modern audio plugin built using the **JUCE 8** framework. It leverages a hybrid architecture that combines a high-performance C++ audio engine with a declarative, web-based User Interface (WebView2).

## Core Architecture
Mushin uses a **Hybrid C++/Web** model:
-   **Backend (C++):** Handles real-time audio processing, DSP (Waveshaping), and parameter management via `AudioProcessorValueTreeState` (APVTS).
-   **Frontend (HTML/CSS/JS):** A minimalist, high-fidelity interface hosted within a `juce::WebBrowserComponent`.
-   **Communication Layer:** 
    -   **JS to C++:** Uses `window.__JUCE__.backend.callNativeFunction` for parameter updates.
    -   **C++ to JS:** Uses `evaluateJavascript` for state sync and `emitEventIfBrowserIsVisible` for high-frequency data (waveforms).

## Tech Stack
-   **Framework:** JUCE 8.0.4
-   **DSP:** Custom Waveshaper implementation with Drive and Exhaustion logic.
-   **Frontend:** Vanilla HTML5, CSS3 (Flexbox/Grid), and JavaScript (Canvas API for visualization).
-   **Rendering:** WebView2 (Edge/Chromium) on Windows.
-   **Build System:** CMake with `juce_add_binary_data` for asset bundling.

## Functional Features
1.  **Gain (Output):** Controls the final output level [0.0 to 2.0].
2.  **Drive:** Input saturation intensity [1.0 to 10.0].
3.  **Threshold:** Sensitivity of the waveshaping effect [0.0 to 1.0].
4.  **Exhaustion:** A boolean toggle affecting the DSP algorithm's character.
5.  **Waveform Visualizer:** Real-time rendering of the mono-summed output signal using an HTML5 Canvas.

## UI Design Philosophy
-   **Minimalism:** Clean lines, generous spacing, and a dark-mode "Inter" font aesthetic.
-   **Responsiveness:** The UI is fully resizable with fixed aspect ratio constraints (1.5:1).
-   **Visual Feedback:** High-contrast blue accents (#00d2ff) for active controls and the live waveform.

## Technical Implementation Details
-   **Asset Bundling:** All web assets (`index.html`, etc.) are compiled into the binary using JUCE's BinaryData to ensure portability and eliminate runtime path issues.
-   **Data Pipe:** A high-speed `AbstractFifo` transfers audio samples from the processing thread to the Message Thread, where they are packaged and emitted to the WebView at 30Hz.
-   **Windows Integration:** Requires `WebView2Loader.dll` alongside the executable for proper component initialization.

## Build & Development Workflow
-   **Primary Build:** CMake targeting `build2` for MSBuild.
-   **UI Iteration:** Frontend changes in `Source/Web/index.html` require a rebuild to update the `BinaryData` bundled in the plugin.
