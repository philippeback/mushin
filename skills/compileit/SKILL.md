---
name: compileit
description: Builds the Mushin project in the build2 directory using MSBuild. Use when the user wants to compile the plugin, update web assets bundled in binary data, or generate a new standalone executable.
---

# Compileit Skill

This skill builds the Mushin project using the established MSBuild/CMake workflow in the `build2` directory.

## Core Workflow

To build the project, execute the bundled PowerShell script:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File skills/compileit/scripts/build.ps1 [target]
```

### Targets

- `Mushin_Standalone`: (Default) Generates the standalone executable.
- `Mushin_VST3`: Generates the VST3 plugin.
- `Mushin_All`: Builds all targets.

### Important Notes

- **WebView2:** Ensure the project is closed before building if you are overwriting the executable, as the linker will fail if the file is in use.
- **BinaryData:** Building the project automatically refreshes the `BinaryData` compiled into the Shared Code library, which is necessary when `Source/Web/index.html` has been modified.
- **Environment:** Requires CMake and MSBuild to be available in the path. The `build2` directory must have been previously configured via CMake.
