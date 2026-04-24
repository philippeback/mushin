# Feature: Graphical Shader Support (Visual Synesthesia)

## Overview
Transform the plugin's visual identity by integrating GLSL shaders that react dynamically to the music's beat, tempo, and audio frequency. The system should support loading shader code compatible with the [Shadertoy](https://www.shadertoy.com/) specification (`mainImage` entry point).

## Architecture

### 1. Rendering Engine
- **Option A (Pure Web):** Use WebGL 2.0 within the `WebView2` component. This is the preferred approach to keep the UI logic consolidated in HTML/JS.
- **Option B (Hybrid):** Use `juce::OpenGLContext` to render a background layer behind the WebView.
- **Decision:** **Option A** is chosen for maximum flexibility and ease of integration with existing CSS panels.

### 2. Shader Toy Compatibility Layer
To support Shadertoy-style code, the following uniforms must be provided to the shader:
- `iTime`: Elapsed time in seconds.
- `iTimeDelta`: Time since last frame.
- `iFrame`: Current frame number.
- `iSampleRate`: Audio sample rate.
- `iResolution`: Window dimensions (width, height).
- `iChannel0..3`: Textures or audio data.
- **Beat-Sync Uniforms:**
    - `iBeat`: Current beat count from host.
    - `iTempo`: BPM from host.
    - `iIsPlaying`: Playback state.

### 3. C++ Backend (Synchronization)
The `MushinAudioProcessor` must push transport data to the UI at 30Hz:
- Override `processBlock` to capture `juce::AudioPlayHead::CurrentPositionInfo`.
- Expose a `getTransportState()` method via the bridge.
- Push high-frequency audio spectrum data (FFT) to the UI to allow shaders to "dance" to specific frequencies.

### 4. Web Frontend Implementation
- **Shader Manager:** A JavaScript module that handles compilation of vertex/fragment shaders.
- **Audio Texture:** Map the FFT data from C++ into a 1D or 2D texture (available as `iChannel0` in the shader) to drive visual modulation.
- **Beat Pulse:** Calculate a `pulse` value (0.0 to 1.0) based on the beat position to create rhythmic flashes or movements.

### 5. UI Controls
- **Shader Selector:** A gallery of pre-defined "Signal Corruption" shaders (e.g., Glitch, Neon Flow, CRT Distortion).
- **Intensity Slider:** Controls the modulation depth of the shader.
- **Shadertoy Import:** A text area to paste raw GLSL code for custom visuals.

## Implementation Steps

1. **Bridge Enhancement:**
    - Update the bridge to include `bpm`, `timeSig`, and `isPlaying` in the data stream.
    - Implement a lightweight FFT in the C++ backend to provide frequency data.
2. **WebGL Boilerplate:**
    - Add a `<canvas id="shaderCanvas">` that covers the background of `index.html`.
    - Create the WebGL program and uniform binding logic.
3. **Beat Sync Logic:**
    - Implement the interpolation logic to ensure smooth animation even between DAW frames.
4. **Validation:**
    - Verify performance impact (GPU usage).
    - Ensure shaders correctly reset when playback restarts.
    - Test compatibility with complex Shadertoy "multipass" logic (if required).
