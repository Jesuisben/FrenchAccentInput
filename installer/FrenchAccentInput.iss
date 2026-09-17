#define AppName "French Accent Input"
#define AppVersion "1.0.0"
#define AppExeName "FrenchAccentInput.exe"
#define AppMutex "Local\FrenchAccentInput-4A67FCE1-5DC0-4ACB-9843-9672E4CBE071"

[Setup]
; 고정 AppId는 다음 버전 installer가 기존 설치를 upgrade하도록 같은 제품 계보를 만든다.
AppId={{4A67FCE1-5DC0-4ACB-9843-9672E4CBE071}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=French Accent Input contributors
DefaultDirName={localappdata}\Programs\FrenchAccentInput
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
; 사용자별 경로만 쓰므로 UAC 관리자 승격이 필요 없다.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir=..\dist
OutputBaseFilename=FrenchAccentInput-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupLogging=yes
Uninstallable=yes
UninstallDisplayIcon={app}\{#AppExeName}
LicenseFile=..\LICENSE
AppMutex={#AppMutex}
; 실행 중인 hook을 둔 채 파일을 교체하거나 제거하지 못하게 같은 mutex를 검사한다.
CloseApplications=yes
RestartApplications=no
VersionInfoVersion=1.0.0.0
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Files]
Source: "..\build\Release\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent
