#define MyAppName "Lume"
#define MyAppVersion "0.1.2"
#define MyAppExeName "lume.exe"

[Setup]
AppId=Lume.Language
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
DefaultDirName={localappdata}\Programs\Lume
DefaultGroupName=Lume
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=..\dist
OutputBaseFilename=Lume-0.1.2-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Lume {#MyAppVersion}
UninstallDisplayIcon={app}\{#MyAppExeName}
LicenseFile=..\LICENSE
ChangesEnvironment=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#if FileExists("..\assets\lume.ico")
SetupIconFile=..\assets\lume.ico
#endif

[Files]
Source: "..\build\windows-package\lume.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\windows-package\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Code]
const
  UserEnvironmentKey = 'Environment';

function NormalizePathEntry(Value: String): String;
begin
  Value := RemoveQuotes(Trim(Value));
  while (Length(Value) > 3) and (Value[Length(Value)] = '\') do
    Delete(Value, Length(Value), 1);
  Result := Lowercase(Value);
end;

function PathContains(const PathValue, Entry: String): Boolean;
var
  Remaining, Item: String;
  Separator: Integer;
begin
  Result := False;
  Remaining := PathValue;
  while Remaining <> '' do begin
    Separator := Pos(';', Remaining);
    if Separator = 0 then begin Item := Remaining; Remaining := ''; end
    else begin Item := Copy(Remaining, 1, Separator - 1); Delete(Remaining, 1, Separator); end;
    if NormalizePathEntry(Item) = NormalizePathEntry(Entry) then begin Result := True; Exit; end;
  end;
end;

procedure AddToUserPath(const Entry: String);
var
  Current: String;
begin
  if not RegQueryStringValue(HKCU, UserEnvironmentKey, 'Path', Current) then Current := '';
  if not PathContains(Current, Entry) then begin
    if (Current <> '') and (Current[Length(Current)] <> ';') then Current := Current + ';';
    RegWriteStringValue(HKCU, UserEnvironmentKey, 'Path', Current + Entry);
  end;
end;

procedure RemoveFromUserPath(const Entry: String);
var
  Current, Remaining, Item, Updated: String;
  Separator: Integer;
begin
  if not RegQueryStringValue(HKCU, UserEnvironmentKey, 'Path', Current) then Exit;
  Remaining := Current;
  Updated := '';
  while Remaining <> '' do begin
    Separator := Pos(';', Remaining);
    if Separator = 0 then begin Item := Remaining; Remaining := ''; end
    else begin Item := Copy(Remaining, 1, Separator - 1); Delete(Remaining, 1, Separator); end;
    if (Item <> '') and (NormalizePathEntry(Item) <> NormalizePathEntry(Entry)) then begin
      if Updated <> '' then Updated := Updated + ';';
      Updated := Updated + Item;
    end;
  end;
  RegWriteStringValue(HKCU, UserEnvironmentKey, 'Path', Updated);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then AddToUserPath(ExpandConstant('{app}'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then RemoveFromUserPath(ExpandConstant('{app}'));
end;
