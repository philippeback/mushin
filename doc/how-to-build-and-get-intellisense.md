# HOWTO: Building Mushin & Configuring IntelliSense (Windows)

This guide provides an exhaustive protocol for setting up the Mushin development environment on Windows 10/11. Following these steps exactly is required to avoid build failures and "red squiggles" in your IDE.

---

## 1. Mandatory Tools

You **must** have the following tools installed. Do not use alternative versions unless you are prepared to manually map environment variables.

### A. Visual Studio Build Tools 2022
*   **Version:** 17.x or later.
*   **Required Workload:** "Desktop development with C++".
*   **Key Binaries:** `cl.exe` (Compiler), `link.exe` (Linker), `nmake.exe` or `msbuild.exe`.
*   **Path Assumption:** `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`

### B. CMake
*   **Installation:** `winget install kitware.cmake`
*   **Requirement:** Must be added to your system `PATH`.

### C. Ninja Build System
*   **Installation:** `winget install Ninja-build.ninja`
*   **Why:** We use Ninja because it generates `compile_commands.json`, which is the "Source of Truth" for IntelliSense.

---

## 2. Environment Initialization

The MSVC compiler (`cl.exe`) **cannot function** in a standard PowerShell or CMD window without initialization. It requires specific environment variables (like `INCLUDE`, `LIB`, and `PATH`) to find the C++ Standard Library (STL) headers (e.g., `<algorithm>`).

### The Golden Command
To initialize the environment for 64-bit architecture, you must call the `vcvars64.bat` script provided by Microsoft.

**Path:** `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`

---

## 3. Project Generation (IntelliSense Fix)

IntelliSense requires a `compile_commands.json` file in the `build/` directory to understand where JUCE and STL headers are located.

### The Protocol:
1.  Open a terminal.
2.  Run the following command to generate the project using the Ninja generator:

```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cmake -B build -G "Ninja" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON'
```

**Why this specific command?**
*   `cmd /c`: Switches to CMD to execute the `.bat` script.
*   `call ...vcvars64.bat`: Loads the compiler environment.
*   `-G "Ninja"`: Forces the use of Ninja (required for the next flag).
*   `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`: Generates the database for IntelliSense.

---

## 4. Building the Project

Building via `cmake --build build` will **fail** in a standard terminal because the compiler won't find the STL. You have two options:

### Option A: The "One-Liner" (Standard Terminal)
Use this if you are working in the default VS Code terminal:
```powershell
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cmake --build build'
```

### Option B: The Developer Shell (Recommended)
1.  Press `Win + S`.
2.  Search for **"Developer PowerShell for VS 2022"**.
3.  Navigate to the project folder.
4.  Run simply: `cmake --build build`

---

## 5. VS Code Configuration (`.vscode`)

To ensure the IDE uses the generated database, verify these files exist:

### `settings.json`
```json
{
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
    "C_Cpp.intelliSenseEngine": "default"
}
```

### `c_cpp_properties.json`
```json
{
    "configurations": [
        {
            "name": "Win32",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "configurationProvider": "ms-vscode.cmake-tools",
            "intelliSenseMode": "windows-msvc-x64"
        }
    ],
    "version": 4
}
```

---

## 6. Troubleshooting "Squiggles"

If red lines appear under `#include <algorithm>` or JUCE headers:
1.  **Check for `build/compile_commands.json`**: If it's missing, repeat Step 3.
2.  **Restart the IntelliSense Engine**: In VS Code, press `Ctrl + Shift + P` and run `C/C++: Restart IntelliSense Process`.
3.  **Verify Compiler Path**: Ensure `vcvars64.bat` was called before generation; otherwise, the paths in `compile_commands.json` will be empty or incorrect.
