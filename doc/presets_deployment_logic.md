# Mushin Presets Deployment Logic & Architecture

## Overview
This document explains the presets deployment system for Mushin, analyzes the challenges associated with standard Windows installer mechanisms, and documents the implemented C++ binary resource extraction system.

---

## 1. Resolved Issue and Root Cause Analysis

### Missing Presets Symptom
When installing Mushin using the generated Inno Setup installer (`Mushin_Windows_Installer_v1.0.0.exe`), the plugin loaded successfully, but the preset dropdown was empty or missing the 23 factory presets and 14 trading presets.

### Root Cause 1: Lack of Preset Copying in Inno Setup
In the initial Inno Setup packaging script [mushin_installer.iss](file:///C:/Dev/github/philippeback/mushin/scripts/packaging/mushin_installer.iss), the installer defined the directory creation for presets under `[Dirs]` but did **not** include any file copying rules in the `[Files]` section to copy the XML preset files.

### Root Cause 2: Profile Mismatch due to Admin Elevation (UAC)
The Inno Setup script requires:
```inno
PrivilegesRequired=admin
```
This is required because the VST3 plugin must be installed into the common Program Files directory (`C:\Program Files\Common Files\VST3`). 
When an installer runs with elevated privileges on Windows:
1. The constant `{userappdata}` expands to the **roaming profile of the Administrator account** running the installation (e.g., `C:\Users\Administrator\AppData\Roaming`).
2. It does **not** expand to the profile of the standard user who initiated the installation (e.g., `C:\Users\username\AppData\Roaming`).
3. When the user launches their DAW (Reaper, Ableton, etc.) as a standard user, the plugin scans the standard user's AppData directory (`C:\Users\username\AppData\Roaming\Mushin\Presets`). Since the installer copied files to the Administrator's AppData, the plugin saw an empty directory.

---

## 2. Technical Architecture: Implemented Binary Asset Compilation

To ensure presets are always reliably deployed to the correct user profile regardless of UAC elevation, we compile the factory presets directly into the plugin binary and unpack them at runtime.

### 1. CMake Compilation ([CMakeLists.txt](file:///C:/Dev/github/philippeback/mushin/CMakeLists.txt))
We define a CMake target `MushinPresets` using JUCE's `juce_add_binary_data` utility. It gathers all preset XML files from the [presets/](file:///C:/Dev/github/philippeback/mushin/presets) and [presets_bank_trading/](file:///C:/Dev/github/philippeback/mushin/presets_bank_trading) directories:

```cmake
file(GLOB PRESET_FILES 
    "presets/*.xml"
    "presets_bank_trading/*.xml"
)

juce_add_binary_data(MushinPresets
    HEADER_NAME "MushinPresets.h"
    NAMESPACE MushinPresetsData
    SOURCES
        ${PRESET_FILES}
)
```
This target is linked to the main target:
```cmake
target_link_libraries(Mushin PRIVATE MushinWebData MushinPresets ...)
```

### 2. Runtime Unpacking ([PresetManager.cpp](file:///C:/Dev/github/philippeback/mushin/Source/PresetManager.cpp))
On initialization, the [PresetManager](file:///C:/Dev/github/philippeback/mushin/Source/PresetManager.h#L6) constructor calls the private helper `unpackFactoryPresets()`:

```cpp
PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
    : apvts(state),
      userPresetDir (juce::File::getSpecialLocation(
                         juce::File::userApplicationDataDirectory)
                         .getChildFile("Mushin")
                         .getChildFile("Presets"))
{
    if (!userPresetDir.exists())
        userPresetDir.createDirectory();
    
    unpackFactoryPresets();
}
```

The unpacking method iterates over the embedded binary assets, checks if they exist in the local user's AppData directory, and writes them out:

```cpp
void PresetManager::unpackFactoryPresets()
{
    juce::Logger::writeToLog ("PresetManager: Unpacking factory presets...");
    for (int i = 0; i < MushinPresetsData::namedResourceListSize; ++i)
    {
        const char* mangledName = MushinPresetsData::namedResourceList[i];
        const char* originalPath = MushinPresetsData::originalFilenames[i];
        
        juce::File originalFile (originalPath);
        juce::String filename = originalFile.getFileName();
        
        juce::File destFile = userPresetDir.getChildFile (filename);
        
        // Unpack only if the preset is not already present in the user directory
        if (!destFile.existsAsFile())
        {
            int dataSize = 0;
            const char* data = MushinPresetsData::getNamedResource (mangledName, dataSize);
            
            if (data != nullptr && dataSize > 0)
            {
                if (destFile.create())
                {
                    destFile.replaceWithData (data, dataSize);
                }
            }
        }
    }
}
```

---

## 3. Advantages of the Implemented Binary Solution
- **UAC Bypass:** Bypasses Windows profile elevation mismatches. The presets are unpacked when the plugin is run inside the host DAW as the standard user, ensuring they are placed in that user's correct local AppData folder.
- **Cross-Platform Compatibility:** The compilation and extraction logic is entirely in C++ (JUCE), working seamlessly on Windows, macOS, and Linux without requiring changes to platform-specific installers.
- **Clean Installer Setup:** Keeps the Inno Setup script simple and uncluttered, leaving only the binaries (`.exe` / `.vst3` / `WebView2Loader.dll`) to be copied.
