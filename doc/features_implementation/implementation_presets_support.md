# Preset Support – Implementation Design  
**Location:** `doc/features_implementation/implementation_presets_support.md`  

---

## 1. Overview

The preset system allows users to:

| Action | What happens |
|--------|--------------|
| **Save** a current state → writes the full `AudioProcessorValueTreeState` (APVTS) to disk as XML. |
| **Load** a preset → replaces the APVTS with the stored tree and updates the UI/host. |
| **List** available presets → returns a JSON array of names for the front‑end. |
| **Delete** a user preset → removes the file from the user directory. |

Factory presets are bundled in `BinaryData` and never written to disk; they are read-only.

---

## 2. Data Format

* **XML** – JUCE’s native `ValueTree` format (`treeState.state.toXmlString()` / `juce::XmlDocument`).  
* Root element: `<Preset>` with attributes:
  * `version="1"` – for future compatibility.
  * `name="…"` – user‑supplied name (factory presets use the binary file name).
* Child elements are the serialized `ValueTree` of the APVTS.

> **Why XML?**  
> • Human readable, easy to debug.  
> • Directly supported by JUCE (`getStateInformation()` / `setStateInformation()`).  

---

## 3. Directory Layout

| Type | Path (Windows) | Path (macOS) | Notes |
|------|----------------|--------------|-------|
| **Factory presets** | `BinaryData::factoryPresetX_xml` | – | Read‑only, embedded in the binary. |
| **User presets** | `%AppData%\Mushin\Presets\` | `~/Library/Application Support/Mushin/Presets/` | Created on first run (`juce::File::createDirectory()`). |

> **Linux support** – use `XDG_DATA_HOME` or fallback to `$HOME/.local/share/Mushin/Presets`.

---

## 4. PresetManager Class

```cpp
// PresetManager.h
class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    // CRUD
    bool savePreset (const juce::String& name, juce::Result& result);
    bool loadPreset (const juce::String& name, juce::Result& result);
    bool deletePreset(const juce::String& name, juce::Result& result);

    // Helpers
    juce::Array<juce::String> getPresetList() const;
    juce::File          getUserPresetDir() const { return userPresetDir; }
    juce::File          getPresetFile (const juce::String& name) const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File userPresetDir;
};
```

### 4.1 Constructor

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
}
```

### 4.2 `savePreset`

```cpp
bool PresetManager::savePreset (const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    juce::XmlDocument xml(apvts.state.toXmlString());
    xml.setAttribute ("name",   name);
    xml.setAttribute ("version","1");

    if (!xml.writeTo(file))
    {
        result = juce::Result::fail("Failed to write preset file");
        return false;
    }
    result = juce::Result::ok();
    return true;
}
```

### 4.3 `loadPreset`

```cpp
bool PresetManager::loadPreset (const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
    {
        result = juce::Result::fail("Preset not found");
        return false;
    }

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse(file));
    if (!xml)
    {
        result = juce::Result::fail("Failed to parse preset XML");
        return false;
    }

    // Optional: check version
    auto ver = xml->getStringAttribute ("version", "0");
    if (ver != "1")
    {
        result = juce::Result::fail("Unsupported preset version");
        return false;
    }

    apvts.state = juce::ValueTree::fromXml(*xml);
    // Notify host of parameter changes
    apvts.stateChanged();
    result = juce::Result::ok();
    return true;
}
```

### 4.4 `deletePreset`

```cpp
bool PresetManager::deletePreset (const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
    {
        result = juce::Result::fail("Preset not found");
        return false;
    }

    if (!file.deleteFile())
    {
        result = juce::Result::fail("Failed to delete preset");
        return false;
    }
    result = juce::Result::ok();
    return true;
}
```

### 4.5 `getPresetList`

```cpp
juce::Array<juce::String> PresetManager::getPresetList() const
{
    juce::Array<juce::String> list;

    // Factory presets (BinaryData)
    for (auto& pair : BinaryData::binaryFiles)   // pseudo‑code – iterate known names
        if (pair.second.endsWith(".xml"))
            list.add(pair.first);

    // User presets
    auto files = userPresetDir.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (auto& f : files)
        list.add(f.getFileNameWithoutExtension());

    return list;
}
```

---

## 5. Integration with `MushinAudioProcessor`

```cpp
class MushinAudioProcessor : public juce::AudioProcessor
{
public:
    // ...
    PresetManager presetMgr { treeState };

    // Expose to WebView2 via callbacks (see section 6)
};
```

### 5.1 Exposing Methods to JavaScript

Use `WebBrowserComponent`’s callback system:

```cpp
// In the editor constructor
webBrowser.registerCallback ("savePreset", [this] (const juce::var& args) {
    auto name = args[0].toString();
    juce::Result r;
    if (presetMgr.savePreset(name, r))
        webBrowser.callJavascriptFunction("onPresetSaved()", {});
    else
        webBrowser.callJavascriptFunction("onPresetError()", {r.getMessage()});
});

webBrowser.registerCallback ("loadPreset", [this] (const juce::var& args) {
    auto name = args[0].toString();
    juce::Result r;
    if (presetMgr.loadPreset(name, r))
        webBrowser.callJavascriptFunction("onPresetLoaded()", {});
    else
        webBrowser.callJavascriptFunction("onPresetError()", {r.getMessage()});
});

webBrowser.registerCallback ("deletePreset", [this] (const juce::var& args) {
    auto name = args[0].toString();
    juce::Result r;
    if (presetMgr.deletePreset(name, r))
        webBrowser.callJavascriptFunction("onPresetDeleted()", {});
    else
        webBrowser.callJavascriptFunction("onPresetError()", {r.getMessage()});
});

webBrowser.registerCallback ("requestPresetList", [this] (const juce::var&) {
    auto list = presetMgr.getPresetList();
    juce::var arr;
    for (auto& n : list)
        arr.append(n);
    webBrowser.callJavascriptFunction("onPresetListReceived()", {arr});
});
```

> **Note:** All callbacks run on the UI thread, so no extra locking is required.

### 5.2 Updating the UI after a load

After `loadPreset()` replaces `treeState.state`, we must:

1. Call `treeState.stateChanged();` – this triggers all parameter listeners (including the editor’s `parameterChanged`).  
2. Emit an event to JS: `emitEvent('presetLoaded', name)`.

---

## 6. Front‑End Integration

| UI Element | Interaction | JS Callback |
|------------|-------------|-------------|
| **Preset dropdown** | On change → `backend.loadPreset(name)` | `onPresetLoaded()` updates the selector and clears “modified” flag. |
| **Save button** | Prompt for name → `backend.savePreset(name)` | `onPresetSaved()` refreshes list. |
| **Delete button** | Confirm → `backend.deletePreset(name)` | `onPresetDeleted()` removes from dropdown. |
| **Modified indicator** | Compare current `treeState.state` with the last loaded preset (store a copy in JS). | Show `*` if different. |

> The front‑end should listen to `presetsUpdated` events and rebuild the selector accordingly.

---

## 7. Thread Safety & Error Handling

| Concern | Solution |
|---------|----------|
| **File I/O on audio thread** | All preset operations are invoked via UI callbacks → safe. If you ever move them off‑thread, wrap with `juce::MessageManagerLock`. |
| **Corrupted XML** | `loadPreset()` returns a `Result` with an error message; JS shows toast. |
| **Missing directory** | Constructor creates the user preset folder if it does not exist. |
| **Duplicate names** | `savePreset()` overwrites existing file – optionally prompt in UI before overwrite. |

---

## 8. Versioning & Migration

* Store a `version` attribute (currently `"1"`).  
* On load, check the version; if unsupported, return an error.  
* Future versions can add new parameters or change structure; implement migration logic inside `loadPreset()`.

---

## 9. Future Enhancements

| Feature | Description |
|---------|-------------|
| **Import/Export** | Let users drag‑drop preset XML files into the app. |
| **Undo/Redo** | Keep a history stack of APVTS states. |
| **Batch operations** | Rename or duplicate presets via UI. |
| **Cloud sync** | Store presets in a cloud folder (e.g., OneDrive). |

---

## 10. Summary

1. Create `PresetManager` to encapsulate all file I/O and XML handling.  
2. Add it as a member of the processor; expose CRUD methods through WebView2 callbacks.  
3. Use JUCE’s `ValueTree` serialization for full state capture.  
4. Keep factory presets read‑only in `BinaryData`; user presets live under `%AppData%/Mushin/Presets`.  
5. Notify JS of changes via events; update UI accordingly.

Implementing the above will give users a robust, cross‑platform preset system that integrates cleanly with both the JUCE backend and the WebView2 front‑end.

## 11. Implementation Files

The following files implement the `PresetManager` class used in the design above.

### src/PresetManager.h
```cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    // CRUD operations
    bool savePreset (const juce::String& name, juce::Result& result);
    bool loadPreset (const juce::String& name, juce::Result& result);
    bool deletePreset(const juce::String& name, juce::Result& result);

    // Helpers
    juce::Array<juce::String> getPresetList() const;
    juce::File          getUserPresetDir() const { return userPresetDir; }
    juce::File          getPresetFile (const juce::String& name) const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File userPresetDir;
};
```

### src/PresetManager.cpp
```cpp
#include "PresetManager.h"

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
    : apvts(state),
      userPresetDir (juce::File::getSpecialLocation(
                         juce::File::userApplicationDataDirectory)
                         .getChildFile("Mushin")
                         .getChildFile("Presets"))
{
    if (!userPresetDir.exists())
        userPresetDir.createDirectory();
}

bool PresetManager::savePreset(const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    juce::XmlDocument xml(apvts.state.toXmlString());
    xml.setAttribute ("name",   name);
    xml.setAttribute ("version","1");

    if (!xml.writeTo(file))
    {
        result = juce::Result::fail("Failed to write preset file");
        return false;
    }
    result = juce::Result::ok();
    return true;
}

bool PresetManager::loadPreset(const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
    {
        result = juce::Result::fail("Preset not found");
        return false;
    }

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse(file));
    if (!xml)
    {
        result = juce::Result::fail("Failed to parse preset XML");
        return false;
    }

    auto ver = xml->getStringAttribute ("version", "0");
    if (ver != "1")
    {
        result = juce::Result::fail("Unsupported preset version");
        return false;
    }

    apvts.state = juce::ValueTree::fromXml(*xml);
    // Notify host of parameter changes
    apvts.stateChanged();
    result = juce::Result::ok();
    return true;
}

bool PresetManager::deletePreset(const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
    {
        result = juce::Result::fail("Preset not found");
        return false;
    }

    if (!file.deleteFile())
    {
        result = juce::Result::fail("Failed to delete preset");
        return false;
    }
    result = juce::Result::ok();
    return true;
}

juce::Array<juce::String> PresetManager::getPresetList() const
{
    juce::Array<juce::String> list;

    // Factory presets (BinaryData) – placeholder: add names manually if known.
    // Example:
    // list.add ("FactoryPreset1");
    // list.add ("FactoryPreset2");

    // User presets
    auto files = userPresetDir.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (auto& f : files)
        list.add(f.getFileNameWithoutExtension());

    return list;
}

juce::File PresetManager::getPresetFile(const juce::String& name) const
{
    return userPresetDir.getChildFile(name + ".xml");
}
```