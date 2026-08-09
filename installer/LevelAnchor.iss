; LevelAnchor installer - Inno Setup 6.3 or newer.
; Ships exactly one payload file: the statically linked VST2 DLL.
;
; Build it with ISCC.exe from this folder. All defines have working defaults,
; so a plain `ISCC LevelAnchor.iss` picks up a Release-static build from the
; usual output path; override them for a real release:
;
;   ISCC /DAppVersion=1.0.0-b260805.2 /DVersionNumeric=1.0.0 ^
;        /DSourceDll=<path to LevelAnchor.dll> /O<output dir>
;
; AppVersion is the full build identity (semver + build ID) and is what names
; the output file. VersionInfoVersion is separate because Windows only accepts
; four numeric fields there, so it gets the bare semver.
;
; Relative paths below resolve against THIS file's folder, one level under the
; repo root.
;
; Two kinds of destination, deliberately separated:
;
;   {app} = Program Files\tlemon.com\LevelAnchor
;     Bookkeeping only - the uninstaller and the terms. A VST folder is
;     scanned by hosts, so nothing but the plugin itself belongs in one; the
;     shared "tlemon.com" root is where other Tlemon products go too.
;
;   The VST folders (see [Tasks]/[Files])
;     Nothing but LevelAnchor.dll. The user ticks one or more; the defaults
;     match what a 64-bit OBS scans - OBS builds a fixed list of six
;     directories (obs-vst/obs-vst.cpp, fill_out_plugins) with no registry key
;     and no user override. Common Files\VST2 is on that list and is also the
;     Steinberg convention, so DAW users find it too. Anything else is covered
;     by the custom-folder option rather than a longer checkbox list.
;
; The licence page shows TERMS.txt, NOT the repo's LICENSE. LICENSE is MIT and
; covers the *source*; this installer hands over a *binary* with LevelAPI2 in
; it, which MIT does not cover and which carries the EUR 500k threshold.
; TERMS.txt states all three layers and reproduces the MIT text in full, so it
; is also what gets installed to {app}. Keep it plain ASCII - Inno reads a
; BOM-less text file as ANSI, so a literal EUR sign would render as mojibake.

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef VersionNumeric
  #define VersionNumeric "1.0.0"
#endif
#ifndef SourceDll
  #define SourceDll "..\LevelAnchor\Builds\VisualStudio2022\x64\Release-static\VST\LevelAnchor.dll"
#endif

[Setup]
; Never change AppId - it is what links an upgrade to the existing install.
AppId={{FBA13553-8EB6-4553-B775-A7E84110E97B}
AppName=LevelAnchor
AppVersion={#AppVersion}
AppVerName=LevelAnchor {#AppVersion}
AppPublisher=Tlemon
AppPublisherURL=https://levelapi.dev/levelanchor/?ref=installer
AppSupportURL=https://levelapi.dev/levelanchor/?ref=installer
; Numeric-only: AppVersion carries a "-b<date>.<n>" build ID that this field
; cannot take.
VersionInfoVersion={#VersionNumeric}

; Relative to this file's folder. See the header for why it is not ..\LICENSE.
LicenseFile=TERMS.txt

; Not where the plugin lands - see the header. The dir page stays off because
; this path is bookkeeping, not a choice worth putting to the user.
DefaultDirName={commonpf64}\tlemon.com\LevelAnchor
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin

; The x64 constants below already force the 64-bit trees, but these keep a
; 32-bit Windows from installing a plugin it can never load.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Let PrepareToInstall handle a running OBS with a clear message instead of
; letting Restart Manager try to close it - this may be a live stream.
CloseApplications=no

OutputBaseFilename=LevelAnchor-{#AppVersion}-Setup
UninstallDisplayName=LevelAnchor {#AppVersion}
; No UninstallDisplayIcon: every plugin location is now optional, so there is
; no file guaranteed to exist to point it at.
WizardStyle=modern
SolidCompression=yes

[Messages]
; The licence page's own wording. LicenseLabel3 sits above the scrolling box
; and is the only part most people read, so it carries the summary rather than
; Inno's "please read the following" boilerplate - the terms themselves are
; three layers deep and nobody should have to scroll to learn the plugin is
; free. It is deliberately not a substitute for the text below it, and
; ExpandLicenseSummary (see [Code]) grows the label to fit.
LicenseLabel=Free to use. A license is only required above EUR 500k of annual revenue.
LicenseLabel3=LevelAnchor is free for individuals and for organizations under EUR 500k of annual revenue, including on monetized streams - nothing to pay, nothing to register, nothing to sign. Above that threshold, or to bundle it into another product, a LevelAPI license is required.%n%nThe plugin's source code is separately MIT licensed; the LevelAPI2 engine inside it is not.

; The tasks page is repurposed as a plain "install to" folder picker, so its
; wording is replaced wholesale. The note goes above the list rather than
; hanging off the first entry, which keeps the entries to bare folder names.
WizardSelectTasks=Install folders
SelectTasksDesc=Where should LevelAnchor be installed?
SelectTasksLabel2=OBS reads the first folder - leave it ticked unless you know your host uses another one.%n%nInstall to:

[Tasks]
Name: vst2; Description: "Program Files\Common Files\VST2"
Name: legacydir; Description: "Program Files\VSTPlugins"; Flags: unchecked
Name: customdir; Description: "Custom folder..."; Flags: unchecked

[Files]
Source: "{#SourceDll}"; DestDir: "{commoncf64}\VST2"; Tasks: vst2; Flags: ignoreversion
Source: "{#SourceDll}"; DestDir: "{commonpf64}\VSTPlugins"; Tasks: legacydir; Flags: ignoreversion
Source: "{#SourceDll}"; DestDir: "{code:GetCustomDir}"; Tasks: customdir; Flags: ignoreversion
Source: "TERMS.txt"; DestDir: "{app}"; DestName: "LevelAnchor-LICENSE.txt"; Flags: ignoreversion

[UninstallDelete]
; Leave the shared company root behind only if another Tlemon product is in it.
Type: dirifempty; Name: "{commonpf64}\tlemon.com"

[Code]
var
  CustomDirPage: TInputDirWizardPage;

// The licence page's summary label (LicenseLabel3) is several lines longer
// than Inno's default, and the label has a fixed height, so the extra lines
// would simply be clipped. Grow it to fit its own text and give the scrolling
// memo below it whatever is left. Doing it in code rather than with fixed
// numbers keeps it correct at any DPI scaling and font size.
procedure ExpandLicenseSummary;
var
  MemoBottom, MemoTop: Integer;
begin
  MemoBottom := WizardForm.LicenseMemo.Top + WizardForm.LicenseMemo.Height;

  // WordWrap is on, so this keeps the width and adds rows downwards.
  WizardForm.LicenseLabel1.AutoSize := True;

  MemoTop := WizardForm.LicenseLabel1.Top + WizardForm.LicenseLabel1.Height
             + ScaleY(8);

  // Keep the memo worth scrolling. If the summary ever outgrows the page, let
  // the label clip rather than let it swallow the terms it is summarizing.
  if MemoTop > MemoBottom - ScaleY(60) then
    MemoTop := MemoBottom - ScaleY(60);

  // Positions are only ever pushed down, never pulled up - the label is taller
  // than the layout expects, never shorter.
  if MemoTop > WizardForm.LicenseMemo.Top then
  begin
    WizardForm.LicenseMemo.Top := MemoTop;
    WizardForm.LicenseMemo.Height := MemoBottom - MemoTop;
  end;
end;

procedure InitializeWizard;
begin
  ExpandLicenseSummary;

  CustomDirPage := CreateInputDirPage(wpSelectTasks,
    'Custom folder', 'Where else should LevelAnchor be installed?',
    'Pick the folder your host scans for VST2 plugins, then click Next.',
    False, '');
  CustomDirPage.Add('');
  CustomDirPage.Values[0] := GetPreviousData('CustomDir', '');
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  SetPreviousData(PreviousDataKey, 'CustomDir', CustomDirPage.Values[0]);
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = CustomDirPage.ID) and (not WizardIsTaskSelected('customdir'));
end;

// Referenced by the [Files] entry's DestDir. The fallback only guards against
// an empty path reaching the file copier; the entry is skipped entirely unless
// the task is ticked, and the task cannot be ticked without a folder.
function GetCustomDir(Param: string): string;
begin
  Result := Trim(CustomDirPage.Values[0]);
  if Result = '' then
    Result := ExpandConstant('{app}');
end;

function NextButtonClick(PageID: Integer): Boolean;
begin
  Result := True;
  if PageID = wpSelectTasks then
  begin
    // Every folder is optional, so nothing stops the user from installing
    // nowhere. Catch it here rather than letting Setup "succeed" silently.
    if not (WizardIsTaskSelected('vst2') or WizardIsTaskSelected('legacydir')
            or WizardIsTaskSelected('customdir')) then
    begin
      MsgBox('Tick at least one folder to install LevelAnchor to.', mbError, MB_OK);
      Result := False;
    end;
  end
  else if PageID = CustomDirPage.ID then
  begin
    if Trim(CustomDirPage.Values[0]) = '' then
    begin
      MsgBox('Pick a folder, or go back and untick "Custom folder".', mbError, MB_OK);
      Result := False;
    end;
  end;
end;

// Inno's Pascal Script has no GetObject, so WMI is reached via SWbemLocator.
// CreateOleObject and GetActiveOleObject are the only COM entry points available.
function IsProcessRunning(const ExeName: string): Boolean;
var
  Locator, Service, Results: Variant;
begin
  Result := False;
  try
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Service := Locator.ConnectServer('', 'root\CIMV2');
    Results := Service.ExecQuery('SELECT * FROM Win32_Process WHERE Name = "' + ExeName + '"');
    Result := (Results.Count > 0);
  except
    // WMI unavailable - fall through and let the file copy fail loudly instead.
    Result := False;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  // OBS keeps the DLL loaded, so replacing it while OBS runs either fails or
  // silently defers to a reboot. Refuse early rather than half-install.
  if IsProcessRunning('obs64.exe') then
    Result := 'OBS Studio is running.' #13#10#13#10
            + 'Close OBS completely, then run this installer again.';
end;
