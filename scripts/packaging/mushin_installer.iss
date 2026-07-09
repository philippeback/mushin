; =====================================================================
; MUSHIN AUDIO SYNTHESIZER // INNO SETUP INSTALLER SCRIPT
; Production-grade installer for VST3 and Standalone formats on Windows
; =====================================================================

#define AppName "Mushin"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define AppPublisher "Mushin Audio"
#define AppURL "https://mushin-audio.web.app"
#define AppExeName "Mushin.exe"

; Relative paths from the scripts/packaging/ directory to the build outputs
#define BuildDir "..\..\build2\Mushin_artefacts\Release"
; (Change "Debug" to "Release" when building production installers)

[Setup]
AppId={{9F8229E0-F9A5-42E3-A65B-60C1F75E4D1D}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DisableDirPage=yes
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
; EULA file (uses standard LICENSE in root)
OutputDir=..\..\build2\installer
OutputBaseFilename=Mushin_Windows_Installer_v{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation (Recommended)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone Application (.exe)"; Types: full custom; Flags: fixed
Name: "vst3"; Description: "VST3 Audio Plugin (.vst3)"; Types: full custom

[Files]
; --- STANDALONE FILES ---
Source: "{#BuildDir}\Standalone\Mushin.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#BuildDir}\Standalone\WebView2Loader.dll"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; --- VST3 FILES ---
; Recursively copy the entire VST3 bundle directory structure
Source: "{#BuildDir}\VST3\Mushin.vst3\*"; DestDir: "{commoncf}\VST3\Mushin.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Ensure the WebView2Loader.dll is placed in the VST3 x86_64-win binary directory
Source: "{#BuildDir}\Standalone\WebView2Loader.dll"; DestDir: "{commoncf}\VST3\Mushin.vst3\Contents\x86_64-win"; Components: vst3; Flags: ignoreversion


[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Components: standalone
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon; Components: standalone

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Dirs]
; Ensure user preset folder is created with correct permissions
Name: "{userappdata}\Mushin\Presets"; Flags: uninsneveruninstall

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent; Components: standalone

[Code]
// GUID for the Evergreen WebView2 Runtime
const WebView2RuntimeGUID = '{F3017226-FE2A-4295-8ABB-7E3E496F3523}';

// Function to check if the Microsoft Edge/WebView2 Runtime is installed
function IsWebView2RuntimeInstalled: Boolean;
var
  RegPath64: String;
  RegPath32: String;
begin
  RegPath64 := 'SOFTWARE\Microsoft\EdgeUpdate\Clients\' + WebView2RuntimeGUID;
  RegPath32 := 'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\' + WebView2RuntimeGUID;

  // Check both HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER registry branches
  Result := RegKeyExists(HKEY_LOCAL_MACHINE, RegPath64) or
            RegKeyExists(HKEY_LOCAL_MACHINE, RegPath32) or
            RegKeyExists(HKEY_CURRENT_USER, RegPath64) or
            RegKeyExists(HKEY_CURRENT_USER, RegPath32);
end;

// Wizard validation event
function NextButtonClick(CurPageID: Integer): Boolean;
var
  ErrorCode: Integer;
  DownloadURL: String;
begin
  Result := True;
  
  // When leaving the License / Welcome page, check for WebView2
  if CurPageID = wpLicense then
  begin
    if not IsWebView2RuntimeInstalled then
    begin
      if MsgBox('Mushin requires the Microsoft Edge WebView2 Runtime to display its user interface.' + #13#10 + #13#10 +
                'This runtime is currently missing from your system.' + #13#10 + #13#10 +
                'Would you like to open the Microsoft download page to install it now?',
                mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDYES then
      begin
        DownloadURL := 'https://developer.microsoft.com/en-us/microsoft-edge/webview2/consumer/';
        ShellExec('open', DownloadURL, '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
      end;
    end;
  end;
end;

// Cleanup helper: optionally preserve user presets during uninstallation
procedure CurUninstallStepChanged(JustAfterAnUninstallStep: TUninstallStep);
begin
  if JustAfterAnUninstallStep = usPostUninstall then
  begin
    if MsgBox('Would you like to keep your custom saved presets located in ' + ExpandConstant('{userappdata}\Mushin') + '?',
              mbConfirmation, MB_YESNO or MB_DEFBUTTON1) = IDNO then
    begin
      // Recursively delete the local user AppData directory
      DelTree(ExpandConstant('{userappdata}\Mushin'), True, True, True);
    end;
  end;
end;
