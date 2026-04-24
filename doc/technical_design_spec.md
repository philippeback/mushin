# Technical Design Specification: Mushin Signal Corruptor

## 1. Architectural Overview
Mushin is a hybrid VST3/Standalone audio plugin built using **JUCE 8.0.4**. It leverages a decoupled architecture where the high-performance audio processing is handled in C++, while the user interface is rendered in a modern web stack (HTML5/CSS3/ES6).

### 1.1 Core Components
- **Audio Engine (C++):** Uses JUCE DSP modules for low-latency signal processing.
- **UI Layer (Web):** A WebView2-backed interface (Microsoft Edge) running inside the plugin window.
- **Hybrid Bridge:** A dual-path communication system ensuring UI-to-Native reliability across restrictive DAW environments.

---

## 2. The Hybrid Communication Bridge
One of Mushin's most critical features is its custom communication bridge, designed to bypass limitations in certain VST3 hosts where standard JUCE 8 native integration fails to bind correctly.

### 2.1 Path A: High-Level Native Integration (Official)
Mushin uses `juce::WebBrowserComponent::Options::withNativeFunction` to register C++ callbacks that are theoretically exposed to the JavaScript `window.juce` object. 

### 2.2 Path B: URL Interception Fallback (Indestructible)
If the high-level bridge fails to initialize, Mushin falls back to a **Custom Scheme Interception** method.
- **Subclassing:** `MushinWebComponent` inherits from `juce::WebBrowserComponent`.
- **Interception:** It overrides `pageAboutToLoad(const String& newURL)`.
- **Parsing:** When the UI sets `window.location.href` to a `mushin://` URL, the C++ code cancels the navigation and parses the query parameters.
- **Example:** `mushin://setParameterValue?id=drive&val=0.5` is parsed in C++ to update the `drive` parameter in real-time.

### 2.3 Bridge Polyfill (JavaScript)
The frontend uses a `MushinNative` wrapper that attempts the official API first and falls back to URL navigation automatically, ensuring the UI remains functional regardless of the host's security context.

---

## 3. DSP Audio Engine
The audio processing is performed in `MushinAudioProcessor::processBlock`.

### 3.1 Signal Flow
1.  **Incoming Buffer:** Audio enters from the DAW.
2.  **Dry Copy:** A temporary copy of the clean signal is stored for the final Mix stage.
3.  **S-Curve Waveshaper:**
    - **Normal Mode:** Applies a `tanh(x * drive)` soft-clipping curve.
    - **Exhaustion Mode:** Switches to a hard-clipping algorithm: `clamp(x * drive, -threshold, threshold)`.
4.  **Resonant Low-Pass Filter:**
    - Uses a `juce::dsp::StateVariableTPTFilter` (Topology Preserving Transform).
    - Provides smooth, analog-style filtering even at high resonance.
5.  **Dry/Wet Mix:** Performs linear interpolation between the clean `dryBuffer` and the processed `buffer`.
6.  **Master Gain:** Applies the final output volume scaling.

### 3.2 Parameter Management
Mushin utilizes `juce::AudioProcessorValueTreeState` (APVTS) for host automation and state persistence. To ensure thread safety and performance:
- Parameters are cached as `std::atomic<float>*` pointers during the processor's construction.
- The audio thread loads these values using `.load()` to avoid blocking or race conditions.

---

## 4. Visualization & UI Sync
### 4.1 Oscilloscope Data Flow
- **Capture:** Every sample in the `processBlock` (post-mix) is pushed into a `juce::AbstractFifo`.
- **Polling:** The `MushinAudioProcessorEditor` runs a 30Hz timer.
- **Transfer:** It reads blocks of 256 samples from the FIFO and converts them to a `juce::Array<juce::var>`.
- **Emission:** Data is pushed to the UI via `webComponent.emitEventIfBrowserIsVisible("waveform", data)`.

### 4.2 UI Updates
The UI uses an HTML5 `<canvas>` element. When a `waveform` event is received, it updates a `Float32Array` and draws the new waveform using `requestAnimationFrame`, creating a smooth, hardware-style oscilloscope effect.

---

## 5. Build & Deployment
### 5.1 Asset Bundling
All web assets (`index.html`, etc.) are converted to C++ byte arrays using `juce_add_binary_data` via CMake. This means the plugin is entirely self-contained; no external files are required at runtime.

### 5.2 Deployment Protocol
- **Standalone:** Compiled to `build2/Mushin_artefacts/Debug/Standalone/`. Requires `WebView2Loader.dll` in the same folder.
- **VST3:** Bundled into a `Mushin.vst3` directory. The deployment script copies this to `C:\Program Files\Common Files\VST3\`, ensuring that any existing instances in a DAW are refreshed.
- **Cache Management:** Each plugin instance uses a unique `UserDataFolder` in the user's `Documents/Mushin_VST3_Final` folder to prevent conflict between multiple instances running in the same DAW project.
