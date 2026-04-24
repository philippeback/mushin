# Mushin JUCE Plugin Setup

We successfully installed the C++ development toolchain and created a functional VST3 / Standalone plugin shell for "Mushin".

## Changes Made
1. **Toolchain Installation**:
   - Installed `Kitware.CMake` and `Microsoft.VisualStudio.2022.BuildTools` using `winget` to provide the required MSVC compilers and build systems.
2. **Project Configuration (`CMakeLists.txt`)**:
   - Configured CMake to automatically download JUCE 8.0.4 via `FetchContent`.
   - Setup targets for both VST3 and Standalone executable formats.
   - Disabled `COPY_PLUGIN_AFTER_BUILD` to allow building without Administrator privileges.
3. **Audio Engine (`PluginProcessor.h` / `PluginProcessor.cpp`)**:
   - Implemented a basic `processBlock` that applies a `Gain` multiplier to the incoming audio.
   - Pushes a mono mix of the audio samples into a lock-free `juce::AbstractFifo` for thread-safe UI communication.
4. **User Interface (`PluginEditor.h` / `PluginEditor.cpp`)**:
   - Built a sleek UI featuring a dark grey background and centered title.
   - Included a `juce::Slider` connected to an `AudioProcessorValueTreeState::SliderAttachment` to control Gain.
   - Utilized a `juce::AudioVisualiserComponent` running on a 30Hz timer, which reads samples from the processor's FIFO to draw a realtime waveform of the incoming audio.

## Validation Results
- The plugin was successfully configured using `cmake -B build`.
- The compilation succeeded using `cmake --build build --config Release` without any MSVC errors.

## Artifacts Generated
You can find the compiled binaries in your project directory:
- **Standalone Executable**: `build/Mushin_artefacts/Release/Standalone/Mushin.exe` (Run this to test the UI and audio directly without a DAW).
- **VST3 Plugin**: `build/Mushin_artefacts/Release/VST3/Mushin.vst3` (Load this in Ableton, FL Studio, Reaper, etc.).
