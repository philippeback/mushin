# Feature: Preset Support (Loading & Saving)

## Overview
Enable users to save, manage, and load their custom sound configurations (presets). This feature integrates the JUCE C++ backend with the WebView2 frontend.

## Architecture

### 1. Data Format
- **Serialization:** Presets will be serialized using `juce::ValueTree` to XML (native JUCE format) or JSON.
- **Payload:** Each preset contains:
    - Version number (for future compatibility).
    - Preset Name.
    - Full state of the `AudioProcessorValueTreeState` (APVTS).

### 2. C++ Backend (MushinAudioProcessor)
- **State Management:** Leverage `treeState.state` (ValueTree) for serialization.
- **File System:**
    - **Factory Presets:** Bundled in `BinaryData`.
    - **User Presets:** Stored in the user's local application data folder.
        - Windows: `%AppData%\Mushin\Presets\`
        - macOS: `~/Library/Application Support/Mushin/Presets/`
- **Native Bridge Methods:**
    - `savePreset(String name)`: Captures APVTS state and writes to disk.
    - `loadPreset(String name)`: Reads file, replaces APVTS state.
    - `getPresetList()`: Returns a JSON array of available preset names to the UI.
    - `deletePreset(String name)`: Removes a user preset from disk.

### 3. Web Frontend (UI)
- **Preset Browser:** A dropdown or list in the `header-area`.
- **Save Button:** Triggers a popup/input to name and save the current state.
- **State Indicator:** Show "Modified" (e.g., `*`) if the current state differs from the loaded preset.

### 4. Communication Protocol
- **C++ to JS:**
    - `emitEvent('presetsUpdated', list)`: Sent when a preset is added or deleted.
    - `emitEvent('presetLoaded', name)`: Confirms a successful load.
- **JS to C++:**
    - `backend.savePreset(name)`
    - `backend.loadPreset(name)`
    - `backend.deletePreset(name)`
    - `backend.requestPresetList()`

## Implementation Steps

1. **Backend:**
    - Create a `PresetManager` class to handle I/O and directory management.
    - Implement `getStateInformation` and `setStateInformation` to use the same logic.
2. **Bridge:**
    - Expose `PresetManager` methods to the `WebBrowserComponent`.
3. **Frontend:**
    - Add a "Preset Selector" component to `index.html`.
    - Style the browser to match the "Signal Corruptor" aesthetic.
4. **Validation:**
    - Verify parameters correctly interpolate when a preset is loaded (utilizing existing `LinearSmoothedValue` logic).
    - Ensure `exhaustion` (bool) and other discrete parameters transition safely.
