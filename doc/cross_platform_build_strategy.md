# Strategy: Cross-Platform Build & Distribution

## The Problem
Native macOS binaries (VST3/AU/Standalone) require the Apple `ld` linker and `codesign` tools, which only run on macOS. Our local hardware (Windows PC + 2009 MBP) is insufficient for modern macOS builds.

---

## 1. Primary Path: GitHub Actions (CI/CD)
The recommended way to generate macOS builds without owning a modern Mac.

### Workflow Configuration
- **Runner:** `macos-latest` (gives access to M1/M3 ARM64 and Intel x64 cross-compilation).
- **Toolchain:** CMake + Ninja.
- **Dependencies:** Scripted download of JUCE and WebView2 headers.

### Automation Script (`.github/workflows/build.yml`)
1.  **Checkout:** Pulls the Mushin source code.
2.  **Setup:** Installs CMake and Ninja.
3.  **Build:** 
    - `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`
    - `cmake --build build --target Mushin_All`
4.  **Sign & Notarize:** (Requires Apple Developer Program membership).
    - Uses `gon` or `notarytool` to submit the binary to Apple.
5.  **Artifacts:** Uploads the `.vst3` and `.dmg` to the GitHub Release page.

---

## 2. Hardware Path: Reviving the 2009 MBP
If physical testing is required (e.g., checking if the WebView2 logic translates correctly to macOS `WKWebView`):

### Procedure
1.  **OpenCore Legacy Patcher:** Install macOS Ventura or Sonoma on the 2009 MBP.
2.  **SSD/RAM Upgrade:** Essential for survival (minimum 8GB RAM + SSD).
3.  **Xcode:** Install the latest compatible Xcode (15.x).
4.  **Local Build:** Use the `compileit` skill logic but adapted for the Mac terminal.

---

## 3. The Hybrid Workflow (The "Killer" Setup)
1.  **Develop on Windows:** Do 95% of the work on your PC using the WebView2 environment.
2.  **Push to GitHub:** Every `git push` triggers the macOS build.
3.  **Download Artifact:** Download the compiled `.vst3` from GitHub and copy it to your 2009 MBP to test basic functionality and UI rendering.

---

## 4. Universal Binary Support
JUCE 8 makes it easy to build **Universal Binaries** (Intel + Apple Silicon). 
- In CMake, we set: `set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")`.
- This ensures that users on modern M1/M2/M3 Macs get native performance while your 2009 MBP can still run the Intel slice.

## Implementation Steps
1.  Create a `.github/workflows/` directory.
2.  Draft a `build_mac.yml` file using a standard JUCE-CMake template.
3.  Configure secrets (Apple ID, Team ID) in GitHub for auto-notarization.
