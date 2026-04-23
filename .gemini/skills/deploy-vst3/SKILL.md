---
name: deploy-vst3
description: Deploys the Mushin VST3 plugin and its dependencies to the standard Windows VST3 location. Use this skill when the plugin has been built and needs to be installed for use in a DAW.
---

# Deploy VST3 Skill

## Overview
This skill automates the deployment of the Mushin VST3 plugin from the build directory to `C:\Program Files\Common Files\VST3\`. It also ensures that the `WebView2Loader.dll` dependency is correctly placed within the VST3 bundle.

## Workflow
1. **Trigger:** User asks to "deploy the VST", "install the plugin", or "transfer the VST to the common folder".
2. **Action:** Execute the `deploy.ps1` script.
3. **Privileges:** The script will automatically request Administrator privileges if not already running as Admin.

## Resources

### scripts/
- **deploy.ps1**: PowerShell script that performs the file copy operations.
