# UI Refresh Rate & Fluidity Optimization Strategy

This document outlines strategies to improve the refresh rate and visual smoothness of the Mushin Web UI, focusing on the waveform visualization and sidechain meter responsiveness.

## 1. High-Frequency Waveform Visualization

Currently, the waveform is pushed from C++ at 30Hz via JSON serialization. To achieve 60Hz+ fluidity, the following optimizations are recommended:

### A. Push-on-VSync Handshake
- **Current**: C++ `Timer` pushes data at fixed intervals.
- **Proposed**: The JavaScript UI uses `requestAnimationFrame` to signal C++ when it is ready to paint.
- **Benefit**: Eliminates "micro-stutter" caused by the mismatch between the C++ timer and the browser's refresh cycle.

### B. Binary Data Transfer (Zero-Copy)
- **Current**: `juce::Array<juce::var>` is converted to a JSON string.
- **Proposed**: Pack raw `float` samples into a Base64-encoded string or use a custom `postMessage` binary format.
- **Benefit**: Bypasses the expensive JSON string allocation and parsing logic, reducing CPU overhead for both the host and the browser.

### C. GPU Acceleration
- **Proposed**: Utilize **WebGL** or **Canvas 2D** instead of SVG or DOM elements for the waveform path.
- **Benefit**: Hardware-accelerated rendering can handle thousands of vertices at 120Hz+ with negligible impact on the main UI thread.

---

## 2. Sidechain & Meter Fluidity

Meter "sluggishness" is often a result of missing transients or UI thread contention.

### A. Peak Tracking & Ballistics (C++ Side)
- **Current**: Instantaneous values are sampled at 30Hz.
- **Proposed**: Implement a peak-hold and "leaky integrator" decay in the C++ processor.
- **Benefit**: Ensures fast transients (which occur between 30Hz snapshots) are captured and displayed with smooth, "analog-style" decay ballistics.

### B. Composite-Only Animations
- **Current**: Updating `width` or `height` of bar elements.
- **Proposed**: Use `transform: scaleX()` or `transform: scaleY()`.
- **Benefit**: Changing layout properties (`width`) forces the browser to recalculate the entire page layout. `transform` only triggers a **Composite** pass on the GPU, which is significantly faster.

### C. Direct DOM Access
- **Proposed**: For high-frequency meter updates, bypass UI frameworks (React/Vue) and use direct DOM manipulation (`element.style.transform = ...`).
- **Benefit**: Avoids the "Virtual DOM" diffing overhead for every meter tick.

---

## 3. Communication Bridge Optimization

### A. Message Batching
- **Proposed**: Combine the waveform data, sidechain levels, and peak indicators into a single `uiState` object per frame.
- **Benefit**: Reduces the frequency of bridge calls, which are the primary bottleneck in hybrid C++/Web applications.

### B. Throttling and Decimation
- **Proposed**: Only send data when changes exceed a specific threshold or decimate the waveform to a visual resolution (e.g., 64 min/max pairs instead of 256 raw samples).
- **Benefit**: Reduces the amount of data crossing the bridge without sacrificing visual quality.

---

## 4. Advanced: Shared Memory

### SharedArrayBuffer
- **Proposed**: Map a block of shared memory between the JUCE host and the WebView2 engine.
- **Benefit**: True zero-latency data transfer where the UI reads the audio buffer directly from memory. This is the ultimate optimization for professional-grade visualization.
