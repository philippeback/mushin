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

