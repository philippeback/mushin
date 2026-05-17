# WebView Implementation Specifics: Studio One 8 (Fender Studio Pro)

## Overview
During development, a critical issue was identified where the WebView2-based UI would fail to display (hanging the initialization process) in **Studio One 8 Pro**, despite working correctly in Studio One 7.

## Problem Description
The `juce::WebBrowserComponent` constructor would hang indefinitely when called directly within the `MushinAudioProcessorEditor` constructor. Logs indicated that the Host called `createEditor()`, but the internal WebView2 initialization never completed.

## Root Cause: Window Handle Race Condition
WebView2 requires a valid native window handle (HWND on Windows) to attach the browser engine. In Studio One 8, the DAW's graphics engine does not fully anchor or stabilize the plugin's native window handle until *after* the Editor constructor has returned. Attempting to initialize the browser during the constructor resulted in a race condition where the engine stalled waiting for a stable parent window.

## Implementation Fixes

### 1. Deferred Initialization (The "Golden" Fix)
Initialization of the `MushinWebComponent` is now deferred by **250ms** using `juce::Timer::callAfterDelay`.
- This allows the constructor to finish and the host to finalize window positioning.
- 250ms was found to be the optimal balance between stability and perceived responsiveness.

### 2. Robust User Data Folder
The browser's cache and state are now explicitly stored in `%LOCALAPPDATA%`:
- Path: `\PhilippeBack\Mushin\WV2_v1`
- This prevents permission errors and file-locking issues common when using the `Documents` folder (often synced by OneDrive).

### 3. Explicit DLL Location
To ensure the plugin always finds its required Microsoft Edge components, the `WebView2Loader.dll` location is explicitly calculated relative to the VST3 executable:
```cpp
auto dllFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                .getSiblingFile("WebView2Loader.dll");
```

## Diagnostics & Troubleshooting
A persistent log is maintained to track the initialization lifecycle:
- **Log Path:** `%LOCALAPPDATA%\PhilippeBack\Mushin\Mushin_Log.txt`
- **Key Indicators:** Look for "Deferred Init" entries to confirm the browser engine started successfully after the window was established.
