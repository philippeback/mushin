# Feature: Cross-Platform Installer Support

## Overview
Automate the installation process for end-users on Windows and macOS. The installer must handle binary placement, dependency bundling (WebView2), and directory initialization for presets.

## 1. Windows Deployment (Inno Setup)

### Target Locations
- **VST3:** `C:\Program Files\Common Files\VST3\Mushin.vst3\`
- **Standalone:** `C:\Program Files\Mushin\Mushin.exe`
- **Presets/Data:** `%AppData%\Mushin\`

### Critical Dependencies
- **WebView2Loader.dll:** Must be bundled alongside `Mushin.exe` and within the `Mushin.vst3/Contents/x86_64-win/` folder.
- **Edge Runtime:** The installer should check if the Evergreen WebView2 Runtime is installed and offer to download it if missing.

### Inno Setup Responsibilities
- Request Administrative privileges.
- Create Start Menu shortcuts for the Standalone version.
- Offer an uninstaller that cleans up the binaries but optionally leaves user presets.

## 2. macOS Deployment (PKG / DMG)

### Target Locations
- **VST3:** `/Library/Audio/Plug-Ins/VST3/Mushin.vst3`
- **AU (Component):** `/Library/Audio/Plug-Ins/Components/Mushin.component`
- **Standalone:** `/Applications/Mushin.app`
- **Presets/Data:** `~/Library/Application Support/Mushin/`

### Critical Tasks
- **App Bundle Structure:** Ensure the `.app` and `.vst3` bundles contain all necessary Plist info.
- **Permissioning:** Ensure the `~/Library/Application Support/Mushin/` folder has correct write permissions for the current user.

## 3. Code Signing & Notarization (Mandatory)

### Windows
- Sign all `.exe`, `.vst3`, and `.dll` files using a **Standard or EV Code Signing Certificate**.
- Prevents "SmartScreen" warnings and "Unknown Publisher" blocks.

### macOS
- **Hardened Runtime:** Enable during the build process.
- **Notarization:** Submit the app/installer to Apple's Notary Service via `altool` or `notarytool`.
- Prevents "Apple could not verify this software for malicious content" errors on modern macOS (Catalina and later).

## 4. Common Installer Logic
- **EULA:** Display the Mushin End User License Agreement.
- **Version Checking:** Prevent downgrading or detect existing installations to offer an "Update" path.
- **Preset Initialization:** Create the default preset directory and copy factory presets if they aren't embedded in the binary.

## Implementation Steps

1. **Automation:**
    - Create a `scripts/packaging/` directory containing Inno Setup scripts (`.iss`) and macOS distribution scripts.
2. **CI/CD Integration:**
    - Update GitHub Actions (or local build scripts) to trigger the packaging process after a successful "Release" build.
3. **Testing:**
    - Verify installations on "clean" machines (no dev tools installed) to ensure all runtime dependencies (Visual C++ Redistributable, Edge Runtime) are accounted for.
