# GEMINI Foundations - Mushin Project

## Technical Stack & Architecture
- **Framework:** JUCE 8.0.4 (Essential for WebView2 integration).
- **Architecture:** Hybrid C++/Web model.
  - **Audio Engine:** High-performance C++ using JUCE DSP.
  - **User Interface:** Modern web-based UI (HTML5, CSS3, ES6 JavaScript).
- **WebView Backend:** Microsoft Edge/WebView2 (Required on Windows).
- **Communication:** Bidirectional via `juce::WebBrowserComponent` native integration.
- **Asset Management:** All web frontend assets are bundled via `juce_add_binary_data` into the plugin binary.
- **Visualization:** 30Hz high-frequency waveform updates pushed from C++ to JS via `emitEventIfBrowserIsVisible`.

## Build Environment
- **Build System:** CMake.
- **Generator:** MSBuild (Visual Studio).
- **Build Directory:** `build2` (Dedicated for active development and binary generation).
- **Dependencies:** Requires `WebView2Loader.dll` in the standalone output folder.

## Operational Mandates
- **Shell Usage:** All shell commands MUST be executed using **PowerShell** (Default version 5 on Windows). Do not use `grep` or Unix-style tools unless explicitly verified. Use `Select-String`, `Get-Content`, etc.
- **UI Iteration:** Any changes to `Source/Web/index.html` require a re-compile to refresh the `BinaryData` compiled into the Shared Code library.
