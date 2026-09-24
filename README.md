# French Accent Input

French Accent Input is a Windows 11 x64 utility for typing lowercase French accented letters in desktop applications. Hold **Left Alt** and press a supported letter. It uses the existing keyboard layout and runs in the system tray.

## Project Overview

The program adds a small set of French accent shortcuts without installing a new keyboard layout. An input router decides which physical keys to handle. Win32 keyboard and mouse hooks observe input; the program sends Unicode characters to the focused control when it can do so safely.

## Features

| Left Alt + key | Characters in order | Repeated press |
| --- | --- | --- |
| A | à â æ | Replaces the previous character and cycles |
| C | ç | Appends another character |
| E | é è ê ë | Replaces the previous character and cycles |
| I | ï î | Replaces the previous character and cycles |
| O | ô œ | Replaces the previous character and cycles |
| U | ù û ü | Replaces the previous character and cycles |
| Y | ÿ | Appends another character |

The program leaves Right Alt/AltGr, Ctrl, Shift, Windows-key combinations, and unsupported Left Alt combinations to the original application. It prevents a second instance in the same user session.

## Usage

1. Run `FrenchAccentInput.exe` and find **French Accent Input** in the notification area.
2. Hold **Left Alt** and press a supported key. For example, repeated `E` presses in one Left Alt session cycle through `é`, `è`, `ê`, `ë` in the same position.
3. Release Left Alt to start a new sequence. Changing focus, moving the caret, or using another key or the mouse also ends the replacement sequence.
4. Right-click the notification-area icon and choose **Exit / 종료** to quit.

With a Korean IME, `A` (alphabetic) mode permits accents. `가` (Hangul composition) mode passes the original keys through. If the program cannot read the Korean IME mode, it conservatively skips accent conversion.

## Installation

Get the `v1.0.0` files from this repository's GitHub **Releases** section:

- `FrenchAccentInput-Setup-1.0.0.exe`: per-user installer; adds Start menu shortcuts and an uninstall entry. No administrator elevation is requested by its configuration.
- `FrenchAccentInput.exe`: standalone program; run it directly without an installer.
- `SHA256SUMS.txt`: SHA-256 hashes of both EXE files. Compare each downloaded file with the matching line, for example with `Get-FileHash .\FrenchAccentInput.exe -Algorithm SHA256` in PowerShell.

The installer uses `%LOCALAPPDATA%\Programs\FrenchAccentInput` by default. Uninstall it through Windows Installed apps or its Start menu uninstall shortcut. It does not configure launch at Windows sign-in. The installer can offer to launch the app after installation.

**Both EXE files are unsigned.** Windows SmartScreen or device policy may warn or block them. Verify the download source and hash before deciding whether to run them.

## Privacy & Security

The app handles keyboard events and focused-control identity in memory to produce accents. The source contains no telemetry, network request, account sign-in, or code that stores typed text or window titles. The installer enables a local setup log for installation troubleshooting. A normal-permission process cannot inject text into elevated apps because of Windows integrity boundaries.

## Tech Stack

C++20; Win32 API (`WH_KEYBOARD_LL`, `WH_MOUSE_LL`, `SendInput`, system tray and single-instance mutex); Windows IME/IMM; CMake and CTest; MSVC and Windows SDK; Inno Setup for the installer. No third-party application library is bundled in the source tree.

## Development Environment

The project was developed with Visual Studio Code on Windows 11 x64. The verified build uses Visual Studio 2026 Build Tools with the **Desktop development with C++** workload, MSVC 19.50, Windows SDK 10.0.26100, bundled CMake 4.2.3, PowerShell, and Inno Setup 7.1.0. CMake requires version 3.24 or newer. VS Code is an editor, not a runtime requirement.

## Verified Environment

Automated Release build, CTest, standalone lifecycle, and installer install/run/uninstall checks passed on Windows 11 Pro x64 build 26200. CTest reports 2/2 tests passed. A user reported physical Left Alt, key-repeat, shortcuts, Korean IME `A`/`가`, Notepad, Chrome textarea, and VS Code checks passed on Windows 11 x64. That physical check used the same input source before the MSVC runtime-linkage change; the current binaries were rebuilt and automatically checked but were not physically retested.

## Runtime / Supported Environment

The documented target is Windows 11 x64, with ordinary desktop input at the same or lower integrity level. The installer requires Windows build 22000 or later and an x64-compatible system. The current EXE statically links the MSVC runtime, so users do not need VS Code, CMake, Build Tools, Inno Setup, or a separate Visual C++ Redistributable to run it. This does not promise compatibility with every application or Windows configuration.

## Project Structure

```text
FrenchAccentInput/
├─ .gitignore                       # Excludes generated files and local-only state from Git
├─ CMakeLists.txt                   # Defines the C++ application and test targets
├─ LICENSE                          # MIT License for this project
├─ README.md                        # English and Korean project documentation
├─ docs/                             # Public usage and troubleshooting documentation
│  └─ TROUBLESHOOTING.md            # User-focused problem-solving steps
├─ include/                          # Public C++ declarations
│  └─ fai/                           # Input core namespace headers
│     └─ input_core.h               # Input events and router declarations
├─ installer/                        # Installer source configuration
│  └─ FrenchAccentInput.iss         # Per-user Inno Setup installer definition
├─ resources/                        # Windows executable resources
│  └─ version.rc                    # Windows EXE version information
├─ scripts/                          # Build and packaging commands
│  └─ build-release.ps1             # Release build, CTest, installer, and checksums
├─ src/                              # Product implementation
│  ├─ input_core.cpp                # Accent mapping and input state machine
│  └─ main.cpp                      # Win32 hooks, output, IME, tray, and lifecycle
└─ tests/                            # Automated tests and explicit integration helpers
   ├─ input_core_tests.cpp          # Router unit tests used by CTest
   ├─ win32_output_tests.cpp        # Mocked Win32 output tests used by CTest
   ├─ virtual_keyboard_driver.cpp   # Explicit synthetic-input integration helper
   └─ masked_restore_host.cpp       # Explicit Alt-restoration integration helper
```

## Build

Install the development tools listed above. In PowerShell at the project root, run:

```powershell
.\scripts\build-release.ps1
```

The script configures an x64 Release build with the Visual Studio 2026 generator, builds the app and CTest targets with MSVC warnings treated as errors, runs CTest, builds the Inno Setup installer, and writes the two EXE files plus `SHA256SUMS.txt` to `dist/`. If CMake is on `PATH`, run `cmake --build build --config Release --target ALL_BUILD` after configuration to build every target, including the explicit integration helpers. The release script can locate Visual Studio's bundled CMake even when it is not on `PATH`.

## Test

The release script runs both CTest cases. If CTest is on `PATH`, repeat them after configuration with `ctest --test-dir build -C Release --output-on-failure`. `input_core` checks accent routing and modifier behavior. `win32_output_contract` records Win32 output calls without sending desktop input. The virtual keyboard driver and test hosts are not CTest cases; they can generate synthetic desktop input when launched explicitly. Automated and synthetic tests do not establish physical keyboard or IME behavior.

## Limitations

- Lowercase accented letters only; no configuration UI or automatic updates.
- Elevated applications, secure desktops, anti-cheat games, remote sessions, and every custom text control are not guaranteed. Output restrictions can prevent accent insertion or replacement.
- Supported accent combinations take priority over the target app's same shortcut in alphabetic mode.
- Unsigned binaries may trigger SmartScreen or be blocked by device policy.
- Physical input was reported on the pre-linkage-change binary; the present artifact has automated, not repeated physical, input evidence.

See [troubleshooting](docs/TROUBLESHOOTING.md) for user checks.

## License

This project's source and documentation use the [MIT License](LICENSE). Inno Setup and Microsoft build/runtime tools have their own terms; the project's MIT License does not replace them. See [Inno Setup's license](https://jrsoftware.org/files/is/license.txt) and [Microsoft's Visual C++ redistribution guidance](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170).

---

# French Accent Input 한국어

French Accent Input은 Windows 11 x64 데스크톱 앱에서 프랑스어 소문자 악상을 입력하는 도구다. **왼쪽 Alt**를 누른 채 지원 문자를 누른다. 기존 keyboard layout을 유지하며 알림 영역에서 실행된다.

## 프로젝트 소개

새 keyboard layout을 설치하지 않고 제한된 프랑스어 악상 단축키를 제공한다. 입력 라우터가 물리 키 처리 여부를 결정한다. Win32 keyboard/mouse hook으로 입력을 관찰하고, 안전한 경우 포커스된 입력칸에 Unicode 문자를 넣는다.

## 주요 기능

| 왼쪽 Alt + 키 | 순서 | 반복 입력 |
| --- | --- | --- |
| A | à â æ | 직전 글자를 교체하며 순환 |
| C | ç | 누를 때마다 추가 |
| E | é è ê ë | 직전 글자를 교체하며 순환 |
| I | ï î | 직전 글자를 교체하며 순환 |
| O | ô œ | 직전 글자를 교체하며 순환 |
| U | ù û ü | 직전 글자를 교체하며 순환 |
| Y | ÿ | 누를 때마다 추가 |

오른쪽 Alt/AltGr, Ctrl, Shift, Windows 키 조합과 지원하지 않는 왼쪽 Alt 조합은 원래 앱에 전달한다. 같은 사용자 세션에서 중복 실행을 막는다.

## 사용법

1. `FrenchAccentInput.exe`를 실행하고 알림 영역의 **French Accent Input** 아이콘을 확인한다.
2. **왼쪽 Alt**를 누른 채 지원 키를 누른다. 같은 Alt 입력 흐름에서 `E`를 반복하면 한 자리의 글자가 `é`, `è`, `ê`, `ë`로 순환한다.
3. 왼쪽 Alt를 떼면 새 순서가 시작된다. 포커스·커서 이동이나 다른 키·마우스 입력도 교체 순서를 끝낸다.
4. 종료할 때 알림 영역 아이콘을 오른쪽 클릭하고 **Exit / 종료**를 선택한다.

한국어 IME `A`(영문) 모드에서는 악상을 입력한다. `가`(한글 조합) 모드에서는 원래 키를 통과시킨다. 한국어 IME 모드를 읽지 못해도 보수적으로 악상 변환을 건너뛴다.

## 설치

이 저장소의 GitHub **Releases**에서 `v1.0.0` 파일을 받는다.

- `FrenchAccentInput-Setup-1.0.0.exe`: 사용자별 설치 프로그램. 시작 메뉴 바로가기와 제거 항목을 만든다. 설정상 관리자 권한 승격을 요청하지 않는다.
- `FrenchAccentInput.exe`: installer 없이 직접 실행하는 standalone 프로그램.
- `SHA256SUMS.txt`: 두 EXE의 SHA-256 값. PowerShell에서 `Get-FileHash .\FrenchAccentInput.exe -Algorithm SHA256` 등으로 받은 파일의 값과 대조한다.

기본 설치 경로는 `%LOCALAPPDATA%\Programs\FrenchAccentInput`이다. Windows 설치된 앱 또는 시작 메뉴의 제거 바로가기에서 제거한다. Windows 로그인 시 자동 시작은 설정하지 않는다. 설치 완료 화면에서는 바로 실행을 선택할 수 있다.

**두 EXE는 모두 서명되지 않았다.** Windows SmartScreen이나 PC 정책이 경고 또는 차단할 수 있다. 실행 여부를 판단하기 전에 다운로드 출처와 hash를 확인한다.

## 개인정보·보안

앱은 악상 입력을 위해 keyboard event와 포커스된 입력칸의 식별 정보를 메모리에서 처리한다. source에는 telemetry, network 요청, 계정 로그인, 입력 문자나 창 제목을 저장하는 코드가 없다. 설치 프로그램은 문제 해결을 위한 로컬 설치 로그를 활성화한다. 일반 권한 프로세스는 Windows 권한 경계 때문에 관리자 권한 앱에 문자를 주입할 수 없다.

## 기술 스택

C++20, Win32 API(`WH_KEYBOARD_LL`, `WH_MOUSE_LL`, `SendInput`, 알림 영역, 단일 실행 mutex), Windows IME/IMM, CMake·CTest, MSVC·Windows SDK, 설치 프로그램용 Inno Setup을 사용한다. 공개 source tree에 제3자 앱 라이브러리를 포함하지 않는다.

## 개발 환경

Windows 11 x64에서 Visual Studio Code를 편집기로 사용했다. 확인된 빌드 환경은 Visual Studio 2026 Build Tools의 **Desktop development with C++** workload, MSVC 19.50, Windows SDK 10.0.26100, 번들 CMake 4.2.3, PowerShell, Inno Setup 7.1.0이다. CMake 최소 버전은 3.24다. VS Code는 편집기이며 실행 요구사항이 아니다.

## 실제 검증 환경

Windows 11 Pro x64 build 26200에서 자동 Release 빌드, CTest, standalone lifecycle, installer 설치·실행·제거를 통과했다. CTest는 2/2 통과했다. 사용자는 Windows 11 x64에서 물리 왼쪽 Alt, 키 반복, 단축키, 한국어 IME `A`/`가`, 메모장, Chrome textarea, VS Code 검증 PASS를 보고했다. 해당 물리 검증은 MSVC 런타임 연결 방식 변경 전 같은 입력 source를 사용했다. 현재 산출물은 재빌드와 자동 검증을 거쳤지만 물리 입력은 다시 시험하지 않았다.

## 실행·지원 환경

문서상 대상은 Windows 11 x64의 일반 권한 데스크톱 입력이다. installer는 Windows build 22000 이상과 x64 호환 시스템을 요구한다. 현재 EXE는 MSVC 런타임을 정적 링크하므로 일반 사용자는 VS Code, CMake, Build Tools, Inno Setup, 별도의 Visual C++ Redistributable이 필요 없다. 모든 앱과 Windows 구성을 보장한다는 뜻은 아니다.

## 프로젝트 구조

```text
FrenchAccentInput/
├─ .gitignore                       # 생성 파일과 로컬 전용 자료를 Git에서 제외
├─ CMakeLists.txt                   # C++ 앱과 자동 테스트 대상 정의
├─ LICENSE                          # 이 프로젝트의 MIT 라이선스
├─ README.md                        # 영문·한국어 프로젝트 안내
├─ docs/                             # 공개 사용·문제 해결 문서
│  └─ TROUBLESHOOTING.md            # 사용자 중심 문제 해결 절차
├─ include/                          # 공개 C++ 선언
│  └─ fai/                           # 입력 코어 이름공간의 헤더
│     └─ input_core.h               # 입력 이벤트와 라우터 선언
├─ installer/                        # 설치 프로그램 설정 파일
│  └─ FrenchAccentInput.iss         # 사용자별 Inno Setup 설치 설정
├─ resources/                        # Windows 실행 파일 리소스
│  └─ version.rc                    # Windows EXE 버전 정보
├─ scripts/                          # 빌드·패키징 명령
│  └─ build-release.ps1             # 릴리스 빌드·CTest·설치 파일·검사값 생성
├─ src/                              # 제품 구현
│  ├─ input_core.cpp                # 악상 매핑과 입력 상태 머신
│  └─ main.cpp                      # Win32 후크·문자 출력·IME·알림 영역·실행 제어
└─ tests/                            # 자동 테스트와 명시적 통합 검증 도구
   ├─ input_core_tests.cpp          # CTest에서 실행하는 라우터 테스트
   ├─ win32_output_tests.cpp        # CTest에서 실행하는 모의 Win32 출력 테스트
   ├─ virtual_keyboard_driver.cpp   # 명시적으로 실행하는 합성 입력 검증 도구
   └─ masked_restore_host.cpp       # 명시적으로 실행하는 Alt 복원 검증 도구
```

## 빌드

위 개발 도구를 설치한다. 프로젝트 root의 PowerShell에서 실행한다.

```powershell
.\scripts\build-release.ps1
```

이 스크립트는 Visual Studio 2026 generator로 x64 Release를 구성하고, MSVC 경고를 오류로 취급해 앱과 CTest 대상을 빌드한다. CTest를 실행한 다음 Inno Setup installer와 두 EXE·`SHA256SUMS.txt`를 `dist/`에 만든다. CMake가 `PATH`에 있으면 구성 후 `cmake --build build --config Release --target ALL_BUILD`로 명시적 통합 검증 도구까지 모든 대상을 빌드할 수 있다. Release 스크립트는 CMake가 `PATH`에 없어도 Visual Studio의 번들 CMake를 찾는다.

## 테스트

Release 스크립트는 CTest 두 항목을 실행한다. CTest가 `PATH`에 있으면 구성 후 `ctest --test-dir build -C Release --output-on-failure`로 다시 실행한다. `input_core`는 악상 routing과 modifier 규칙을 검사한다. `win32_output_contract`는 실제 데스크톱 입력 없이 Win32 출력 호출을 기록해 검사한다. virtual keyboard driver와 test host는 CTest 대상이 아니며 명시적으로 실행하면 합성 데스크톱 입력을 만들 수 있다. 자동·합성 테스트는 물리 키보드나 실제 IME 성공 증거가 아니다.

## 한계

- 소문자 악상만 제공한다. 설정 UI와 자동 업데이트는 없다.
- 관리자 권한 앱, 보안 데스크톱, anti-cheat 게임, 원격 세션, 모든 사용자 정의 입력칸의 동작은 보장하지 않는다. 출력 제한이 악상 삽입·교체를 막을 수 있다.
- 영문 입력 모드에서 지원 악상 조합은 대상 앱의 동일 단축키보다 우선한다.
- 미서명 실행 파일은 SmartScreen 경고나 PC 정책에 따른 차단을 받을 수 있다.
- 물리 입력 PASS는 링크 변경 전 binary의 보고다. 현재 artifact의 입력 증거는 자동 검증이며 물리 검증을 반복하지 않았다.

사용자 점검 절차는 [문제 해결 문서](docs/TROUBLESHOOTING.md)를 참고한다.

## 라이선스

이 프로젝트의 source와 문서는 [MIT License](LICENSE)를 사용한다. Inno Setup과 Microsoft 빌드·런타임 도구에는 별도 조건이 적용되며, 프로젝트 MIT가 이를 대신하지 않는다. [Inno Setup 라이선스](https://jrsoftware.org/files/is/license.txt)와 [Microsoft Visual C++ 재배포 안내](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170)를 확인한다.
