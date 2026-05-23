# Feature: Cross-Platform Installer Support

This document details the concrete, automated installation architecture for end-users on Windows and macOS, featuring the fully implemented Inno Setup framework for Windows.

---

## 1. Windows Deployment (Inno Setup)

We have created and configured a production-ready Inno Setup Script: **[mushin_installer.iss](file:///C:/Dev/github/philippeback/mushin/scripts/packaging/mushin_installer.iss)**.

### Target Locations
* **VST3 Plugin**: `C:\Program Files\Common Files\VST3\Mushin.vst3\`
* **Standalone application**: `C:\Program Files\Mushin\Mushin.exe`
* **User Presets & Skins**: `%AppData%\Mushin\Presets\`

### WebView2 Dependency Check
Because Mushin is a hybrid C++/Web architecture, the **Evergreen WebView2 Runtime** is a mandatory prerequisite. 
The installer script incorporates custom Pascal scripting inside Inno Setup to verify system readiness by checking registry entries for the Microsoft Edge Update clients:
```pascal
const WebView2RuntimeGUID = '{F3017226-FE2A-4295-8ABB-7E3E496F3523}';
```
* **Behavior**: If the GUID is missing from both `HKEY_LOCAL_MACHINE` and `HKEY_CURRENT_USER` registry hives, the installer prompts the user with an option to open Microsoft’s official consumer download portal to resolve the dependency immediately.
* **WebView2Loader.dll Bundling**: The installer automatically packages the required `WebView2Loader.dll` alongside the Standalone `.exe` *and* inside the VST3 bundle sub-architecture:
  * Destination: `{commoncf}\VST3\Mushin.vst3\Contents\x86_64-win\WebView2Loader.dll`

### Clean Presets Uninstaller Prompt
During uninstallation, a confirmation dialog prompts the user whether they would like to preserve their custom saved presets in `%AppData%\Mushin` or perform a complete purge.

---

## 2. macOS Deployment (PKG / DMG)

For the macOS ecosystem, we establish a two-stage packaging pipeline using native shell utilities.

### Target Locations
* **VST3 Plugin**: `/Library/Audio/Plug-Ins/VST3/Mushin.vst3`
* **AU (Component)**: `/Library/Audio/Plug-Ins/Components/Mushin.component`
* **Standalone App**: `/Applications/Mushin.app`
* **User Presets**: `~/Library/Application Support/Mushin/`

### CLI Packaging Automation (`scripts/packaging/build_mac_installer.sh`)
```bash
#!/bin/bash
set -e

# Compile individual package components
pkgbuild --component "build/Mushin_artefacts/Release/Standalone/Mushin.app" \
         --install-location "/Applications" \
         "build/MushinApp.pkg"

pkgbuild --component "build/Mushin_artefacts/Release/VST3/Mushin.vst3" \
         --install-location "/Library/Audio/Plug-Ins/VST3" \
         "build/MushinVST3.pkg"

# Create a unified installer distribution
productbuild --distribution "scripts/packaging/distribution.xml" \
             --resources "scripts/packaging/resources" \
             --package-path "build" \
             "build/Mushin_macOS_Installer_v1.0.0.pkg"
```

---

## 3. Code Signing & Notarization

To prevent OS-level warnings ("SmartScreen Blocked" or "Apple cannot verify this software"), binaries must be signed as part of the release build system.

### Windows (SignTool)
```powershell
# Sign the binaries before compiling Inno Setup
SignTool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a "build/Mushin_artefacts/Release/Standalone/Mushin.exe"
SignTool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a "build/Mushin_artefacts/Release/VST3/Mushin.vst3/Contents/x86_64-win/Mushin.vst3"
```

### macOS (codesign & notarytool)
```bash
# Enable Hardened Runtime and sign
codesign --force --options runtime --sign "Developer ID Application: YourName" "build/Mushin.app"

# Submit to Apple Notary Service
xcrun notarytool submit "build/Mushin_macOS_Installer.pkg" \
                     --apple-id "developer@email.com" \
                     --password "xxxx-xxxx-xxxx-xxxx" \
                     --team-id "TEAMID" \
                     --wait
```

---

## 4. Build & Compilation Plan

### Windows Installer Compilation
To compile the Windows installer via the command line (assuming Inno Setup is installed):
```powershell
ISCC.exe scripts/packaging/mushin_installer.iss
```
The output file `Mushin_Windows_Installer_v1.0.0.exe` is generated under `build2/installer/`, ready for release!
