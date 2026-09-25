#define AppName "French Accent Input"
#define AppVersion "1.1.1"
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
SetupIconFile=..\resources\FrenchAccentInput.ico
SetupLogging=yes
Uninstallable=yes
UninstallDisplayIcon={app}\{#AppExeName}
LicenseFile=..\LICENSE
AppMutex={#AppMutex}
; 실행 중인 hook을 둔 채 파일을 교체하거나 제거하지 못하게 같은 mutex를 검사한다.
CloseApplications=yes
RestartApplications=no
VersionInfoVersion=1.1.1.0
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
UsePreviousTasks=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Files]
Source: "..\build\Release\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Tasks]
Name: "startmenuicon"; Description: "시작 메뉴 바로가기 만들기"; Flags: unchecked
Name: "desktopicon"; Description: "바탕화면 바로가기 만들기"

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: startmenuicon
Name: "{userdesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Parameters: "--enable-autostart"; Description: "Windows 로그인 시 자동 실행"; Flags: postinstall skipifsilent unchecked runhidden
Filename: "{app}\{#AppExeName}"; Description: "French Accent Input 실행"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{app}\{#AppExeName}"; Parameters: "--disable-autostart"; Flags: runhidden; RunOnceId: "RemoveCurrentUserAutostart"
