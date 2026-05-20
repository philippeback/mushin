# UI Skin Implementation & Persistence

This document outlines the technical implementation of the UI skin (theme) system and its global persistence mechanism in Mushin.

## Architecture Overview

The skin system is designed to be globally persistent across all instances of the plugin and the standalone application. It bypasses the per-instance `AudioProcessorValueTreeState` to ensure a consistent user experience across different DAW projects and sessions.

### Components

1.  **SkinStorage (`Source/SkinStorage.h`)**
    - A static utility class that manages a global `settings.ini` file.
    - Located in the user's application data directory:
        - Windows: `%APPDATA%\MushinPlugin\settings.ini`
        - macOS: `~/Library/Application Support/MushinPlugin/settings.ini`
    - Uses JUCE's `PropertiesFile` API to store the active theme name as a string (e.g., `"Industrial"`, `"Synthwave"`).

2.  **PluginEditor (`Source/PluginEditor.cpp`)**
    - **Initialization**: Loads the saved theme name from `SkinStorage` during the constructor and assigns it to `currentTheme`.
    - **Bridge Integration**:
        - Listens for `setTheme?name=...` calls from the JavaScript frontend.
        - Updates the global `SkinStorage` immediately upon theme change.
        - Triggers a CSS reload in the WebView by appending a timestamp to the `skin.css` request.
    - **Synchronization**: In `syncAllParameters()`, the editor explicitly pushes the `currentTheme` string to the UI via the `applyThemeFromNative` JS function.

3.  **Frontend Bridge (`Source/Web/index.html`)**
    - **applyThemeFromNative**: A globally exposed JS function that C++ calls to set the UI state (e.g., updating the theme dropdown).
    - **setTheme**: A JS function called when the user selects a theme, which sends a `mushin://setTheme?name=...` request to the C++ backend.

## Persistence Logic

Unlike audio parameters which are saved per-instance in the DAW project, the UI skin is treated as a "Global Preference".

- **When the plugin loads**: It reads the global INI file. If no file exists, it defaults to `"Industrial"`.
- **When the skin changes**: It writes to the global INI file immediately.
- **Across instances**: All open instances share the same global state, but each instance only updates its own UI when it receives a command from its own bridge or during its own initialization.

## Startup Synchronization

To avoid race conditions where the WebView loads after the C++ initialization, the following handshake is used:
1.  The `WebBrowserComponent` calls `syncAllParameters()` when the page finishes loading.
2.  `syncAllParameters()` calls `applyThemeFromNative(currentTheme)` via `juce::MessageManager::callAsync`.
3.  The UI receives the theme name and updates its dropdown and CSS links accordingly.

## Support for New Themes

To add a new theme:
1.  Add the theme name to the `<select id="themeList">` in `index.html`.
2.  Add the corresponding CSS variables in the `ResourceProvider` logic within `PluginEditor.cpp` (inside the `skin.css` request handler).
3.  No changes are required to the persistence logic as it handles arbitrary strings.
