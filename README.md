# French Accent Input

> **불어불문학 전공 시절 겪었던 프랑스어 입력의 불편함을, OpenAI Codex를 활용해 실제로 설치·사용할 수 있는 Windows 프로그램으로 구현한 개인 프로젝트입니다.**

기존 영문 키보드 배열을 바꾸지 않고 **왼쪽 Alt + 알파벳** 조합으로 프랑스어 악상 문자를 입력할 수 있습니다.

**[최신 버전 다운로드](https://github.com/Jesuisben/FrenchAccentInput/releases/latest)** · **[한국어 사용 안내](#사용-안내-한국어)** · **[English Guide](#english-guide)**

---

## 프로젝트 제작 배경 1 - 대학 시절부터 해결하고 싶었던 프랑스어 입력의 불편함

저는 대학교에서 불어불문학을 전공하면서 Windows 환경에서 프랑스어를 입력할 때 불편함을 자주 느꼈습니다.

프랑스어 키보드로 전환하면 기존에 익숙했던 영문 키보드와 키 배열이 달라져 적응하기 어려웠고, 프랑스어 특유의 악상 기호(`é`, `è`, `ê`, `à`, `ç` 등)를 입력하는 과정도 번거로웠습니다.

당시 이미 개발자를 진로로 정했던 저는 '**언젠가 개발을 배우게 된다면, 기존 키보드 배열을 바꾸지 않고도 프랑스어를 편하게 입력할 수 있는 프로그램을 만들고 싶다**'고 생각했습니다.

이후 국비지원 개발자 교육과정을 수료하면서 대학 시절부터 생각해 왔던 아이디어를 실제 프로그램으로 구현하고자 했습니다.

**제가 직접 경험한 문제를 해결하고 싶다는 생각**이 이 프로젝트의 첫 번째 출발점이었습니다.

## 프로젝트 제작 배경 2 - AI 활용 역량을 실제 결과물로 보여주기 위해

개발자로 취업을 준비하면서 **AI 코딩 도구를 개발 과정에 활용하는 능력도 중요한 역량이 될 수 있다**고 생각했습니다.

하지만 단순히 AI 도구를 사용할 줄 안다고 설명하는 것만으로는 실제 활용 경험을 보여주기 어렵다고 느꼈습니다.

그래서 제가 직접 발견한 문제를 바탕으로 요구사항을 정하고, AI를 활용해 실제로 동작하는 소프트웨어를 만드는 과정을 하나의 프로젝트로 보여주고 싶었습니다.

대학 시절부터 만들고 싶었던 프랑스어 입력 프로그램을 주제로 선택한 이유도 여기에 있습니다.

단순히 코드를 생성하고 실행하는 수준에서 끝내지 않고 **Windows 환경에서의 동작 확인, 문제 개선, 설치 프로그램 제작, GitHub Releases 배포**까지 진행했습니다.

또한 채용담당자가 실제 결과물과 작업 과정을 확인할 수 있도록 GitHub에 소스코드와 문서를 공개했습니다.

이 프로젝트는 **오랫동안 해결하고 싶었던 개인적인 문제를 해결하는 동시에, AI 코딩 도구의 활용 경험을 구체적인 결과물로 보여주기 위해 진행한 프로젝트**입니다.

## AI를 활용한 개발 과정 - 제가 맡은 역할과 Codex가 수행한 작업

이 프로젝트는 **OpenAI Codex를 활용해 개발한 개인 프로젝트**입니다.

저는 기존 영문 키보드 배열을 그대로 유지하고, `왼쪽 Alt + 알파벳`으로 프랑스어 악상 문자를 입력할 수 있어야 한다는 요구사항을 전달했습니다.

**소스코드 작성과 구현은 Codex에 맡겼고**, 저는 원하는 기능과 동작 방식, 사용 중 발견한 문제를 설명하며 개선을 요청했습니다.

테스트 역시 가능한 부분은 Codex가 수행하도록 했습니다.

특히 **Computer Use**를 통해 AI가 제 컴퓨터를 조작하며 빌드·실행·동작 확인 작업을 최대한 진행하도록 했고, 물리 키보드 입력처럼 제가 반드시 확인해야 하는 부분은 직접 테스트했습니다.

개발 과정에서 **일부 프로그램의 기존 Alt 키 단축키가 먼저 동작하여 악상 문자가 입력되지 않는 문제**가 나타났습니다.

저는 해당 문제 상황과 기대하는 동작을 Codex에 전달하고 개선을 요청했습니다.

이후 지원하는 악상 입력 조합을 처리하고 기존 Alt 동작과의 충돌을 줄이도록 구현을 보완했으며, 실제 입력 동작을 다시 확인했습니다.

다만 Windows 권한 제한과 프로그램별 입력 처리 방식이 다르므로 **모든 애플리케이션에서 동일하게 동작한다고 보장하지는 않습니다.**

## 설치 프로그램 제작 및 GitHub Releases 배포

프로그램을 개발하는 데서 그치지 않고 **다른 사용자도 개발 환경 없이 설치하고 사용할 수 있는 형태로 배포하는 것**까지 목표로 삼았습니다.

Codex를 활용해 **Inno Setup 기반의 Windows 설치 마법사**를 구성하여 설치·제거, 바로가기 생성, Windows 로그인 시 자동 실행 옵션 등을 제공하도록 했습니다.

또한 이번 프로젝트를 통해 **GitHub Releases를 처음 사용하여 프로그램을 배포**했습니다.

사용자는 Releases에서 설치 프로그램 또는 설치 없이 실행할 수 있는 독립 실행 파일을 다운로드할 수 있습니다.

**[GitHub Releases에서 설치 파일 확인하기](https://github.com/Jesuisben/FrenchAccentInput/releases/latest)**

## 이 프로젝트를 통해 보여주고 싶은 것

이 프로젝트는 대학 시절 실제로 겪었던 불편함에서 출발해, 개발을 배운 뒤 AI를 활용하여 오랫동안 생각했던 아이디어를 소프트웨어로 만든 경험입니다.

**AI가 코드를 작성하더라도 해결할 문제와 요구사항을 명확하게 전달하고, 실제 결과물을 확인하며 개선을 반복하는 과정이 중요하다는 점**을 경험했습니다.

또한 설치 프로그램 제작과 GitHub Releases 배포까지 진행하면서 아이디어를 다른 사람도 사용할 수 있는 결과물로 만들어 보았습니다.

---

## 사용 안내 (한국어)

### 프로그램 개요

French Accent Input은 **Windows 11 x64** 환경에서 사용하는 프랑스어 악상 입력 도구입니다.

새 키보드 레이아웃을 설치하거나 현재 배열을 바꿀 필요 없이 **왼쪽 Alt**와 지원하는 알파벳 키를 함께 눌러 악상 문자를 입력합니다.

프로그램은 Windows 알림 영역(시스템 트레이)에서 실행되며, Caps Lock과 Shift 상태에 따라 소문자·대문자 악상을 구분합니다.

### 빠른 설치

1. **[최신 GitHub Release](https://github.com/Jesuisben/FrenchAccentInput/releases/latest)** 페이지를 엽니다.
2. **Assets**에서 `FrenchAccentInput-Setup-1.1.1.exe`를 다운로드합니다.
3. 다운로드한 설치 파일을 실행하고 설치 마법사의 안내를 따릅니다.
4. 설치가 끝나면 **French Accent Input**을 실행합니다.

사용법 창을 닫아도 프로그램은 종료되지 않고 Windows 알림 영역에서 계속 실행됩니다.

설치 없이 실행하려면 같은 Release의 `FrenchAccentInput.exe`를 다운로드하면 됩니다.

`SHA256SUMS.txt`는 다운로드한 실행 파일의 무결성을 확인하기 위한 해시 목록이며 실행 프로그램이 아닙니다.

**주의:** 현재 실행 파일에는 코드 서명이 없어 Windows SmartScreen 경고가 나타날 수 있습니다.

실행 여부를 결정하기 전에 공식 GitHub Release에서 내려받은 파일인지 확인하시기 바랍니다.

### 주요 기능

| 왼쪽 Alt + 키 | 소문자 입력 순서 | 대문자 입력 순서 | 반복 입력 시 동작 |
| --- | --- | --- | --- |
| `a` | à → â → æ | À → Â → Æ | 직전 문자를 교체하며 순환 |
| `c` | ç | Ç | 새 문자 추가 |
| `e` | é → è → ê → ë | É → È → Ê → Ë | 직전 문자를 교체하며 순환 |
| `i` | ï → î | Ï → Î | 직전 문자를 교체하며 순환 |
| `o` | ô → œ | Ô → Œ | 직전 문자를 교체하며 순환 |
| `u` | ù → û → ü | Ù → Û → Ü | 직전 문자를 교체하며 순환 |
| `y` | ÿ | Ÿ | 새 문자 추가 |

Caps Lock이 꺼져 있을 때는 **왼쪽 Alt**로 소문자, **Shift + 왼쪽 Alt**로 대문자를 입력합니다.

Caps Lock이 켜져 있으면 대소문자 동작이 반대로 바뀝니다.

같은 Alt 입력 흐름에서 `e`를 반복해서 누르면 직전 문자가 `é → è → ê → ë` 순서로 바뀝니다.

왼쪽 Alt를 놓거나 포커스·커서 위치가 바뀌면 새로운 입력 흐름이 시작됩니다.

오른쪽 Alt/AltGr, Ctrl·Windows 키 조합 및 지원하지 않는 왼쪽 Alt 조합은 원래 애플리케이션에 전달하도록 구성했습니다.

### 사용 방법

1. 프로그램을 실행한 뒤 Windows 알림 영역에 아이콘이 표시되는지 확인합니다.
2. 메모장 등 텍스트 입력이 가능한 프로그램에서 **왼쪽 Alt + 지원 알파벳 키**를 누릅니다.
3. 같은 키를 반복해 누르면 지원하는 악상 문자가 순서대로 바뀝니다.
4. Windows 알림 영역 아이콘을 왼쪽 클릭하면 사용법 창을 다시 열 수 있습니다.
5. 아이콘을 오른쪽 클릭하면 **사용법 / 설정 / 정보 / 종료** 메뉴를 사용할 수 있습니다.

설정에서는 현재 사용자의 Windows 로그인 시 자동 실행 여부를 변경할 수 있습니다.

프로그램을 완전히 종료하려면 알림 영역 아이콘을 오른쪽 클릭하고 **종료**를 선택합니다.

한국어 IME의 `A`(영문) 모드에서는 악상 입력을 지원하며, `가`(한글 조합) 모드에서는 한글 입력을 보호하기 위해 기존 키 입력을 그대로 전달합니다.

IME 상태를 확인할 수 없는 경우에도 보수적으로 악상 변환을 건너뜁니다.

### 설치 및 제거

설치 마법사는 현재 사용자 계정에 설치하는 방식이며, 기본 설치 경로는 `%LOCALAPPDATA%\Programs\FrenchAccentInput`입니다.

설치 과정에서 시작 메뉴·바탕화면 바로가기 생성 여부를 선택할 수 있습니다.

설치 완료 화면에서는 프로그램 즉시 실행 및 Windows 로그인 시 자동 실행 옵션을 선택할 수 있습니다.

프로그램 제거는 Windows의 **설치된 앱**에서 진행할 수 있습니다.

### 지원 환경 및 제한 사항

- 대상 환경: **Windows 11 x64**.
- 일반 사용자 권한으로 실행하며, 관리자 권한 애플리케이션이나 보안 데스크톱에서의 입력은 지원을 보장하지 않습니다.
- 일부 사용자 정의 입력칸, 원격 세션, 게임 및 보안 프로그램에서는 입력 제한이 있을 수 있습니다.
- 지원하는 왼쪽 Alt 조합은 영문 입력 모드에서 대상 프로그램의 동일한 단축키보다 우선하도록 처리합니다.
- 자동 업데이트 기능은 제공하지 않습니다.
- 실행 파일은 코드 서명이 되어 있지 않아 SmartScreen 또는 기기 정책에 따라 경고·차단될 수 있습니다.

문제가 발생하면 **[문제 해결 가이드](docs/TROUBLESHOOTING.md)** 문서를 참고할 수 있습니다.

### 개인정보 및 보안

프로그램은 악상 입력을 위해 키보드 이벤트와 입력 대상 정보를 메모리에서 처리합니다.

현재 공개된 소스코드에는 입력한 문장이나 창 제목을 저장하는 기능, 계정 로그인, 네트워크 전송 또는 텔레메트리 기능이 포함되어 있지 않습니다.

설치 프로그램은 설치 문제를 파악하기 위한 로컬 로그를 남길 수 있습니다.

---

## 개발 및 검증 정보

### 기술 스택

| 구분 | 사용 기술 |
| --- | --- |
| 언어 | C++20 |
| Windows 입력 처리 | Win32 API (`WH_KEYBOARD_LL`, `WH_MOUSE_LL`, `SendInput`) |
| 한글 입력 상태 확인 | Windows IME / IMM |
| 빌드 및 테스트 | CMake, CTest, MSVC, Windows SDK |
| 설치 프로그램 | Inno Setup |
| 배포 | GitHub Releases |

**기술 스택은 프로그램에 실제로 사용된 기술을 뜻하며, 소스코드 작성 및 구현은 Codex를 통해 진행했습니다.**

### 프로젝트 구조

```text
FrenchAccentInput/
├─ CMakeLists.txt                 # C++ 애플리케이션과 테스트 빌드 구성
├─ README.md                      # 프로젝트 소개 및 사용·개발 문서
├─ LICENSE                        # MIT License
├─ docs/
│  └─ TROUBLESHOOTING.md          # 문제 해결 가이드
├─ include/fai/
│  └─ input_core.h                # 입력 라우터 선언
├─ installer/
│  └─ FrenchAccentInput.iss       # Inno Setup 설치 설정
├─ resources/                     # 아이콘 및 버전 정보
├─ scripts/
│  └─ build-release.ps1          # 릴리스 빌드·테스트·패키징
├─ src/
│  ├─ input_core.cpp             # 악상 매핑 및 입력 상태 처리
│  └─ main.cpp                   # Win32 입력·출력, IME, 트레이
└─ tests/
   ├─ input_core_tests.cpp       # 입력 라우터 자동 테스트
   ├─ win32_output_tests.cpp     # Win32 출력 계약 테스트
   ├─ virtual_keyboard_driver.cpp
   └─ masked_restore_host.cpp
```

### 빌드

Windows 11 x64에서 Visual Studio 2026 Build Tools의 **Desktop development with C++**, Windows SDK, CMake, PowerShell 및 Inno Setup을 준비합니다.

저장소 루트 디렉터리의 PowerShell에서 다음 명령을 실행합니다.

```powershell
.\scripts\build-release.ps1
```

스크립트는 x64 Release 빌드, CTest, 설치 파일 빌드, SHA-256 해시 생성을 순서대로 수행하며 산출물을 `dist/`에 배치합니다.

실제 검증에 사용한 도구는 MSVC 19.50, Windows SDK 10.0.26100, CMake 4.2.3 및 Inno Setup 7.1.0입니다.

CMake는 최소 3.24 버전 이상을 요구합니다.

### 테스트와 확인 범위

Windows 11 Pro x64 build 26200에서 v1.1.1 소스의 경고를 오류로 취급한 Release 빌드, CTest **2/2**, 설치 프로그램 컴파일이 통과한 것으로 기록되어 있습니다.

물리 키보드 검증에서는 왼쪽 Alt, 연속 입력, 관련 단축키, 한국어 IME `A`/`가` 모드, 메모장, Chrome 텍스트 입력칸 및 VS Code를 확인했습니다.

자동 테스트는 입력 라우팅, 대소문자 처리 및 Win32 출력 호출을 검증합니다.

통합 테스트 도구를 명시적으로 실행하면 가상 키보드 입력을 이용한 검증도 가능합니다.

**자동·가상 입력 테스트 결과가 모든 실제 키보드나 모든 Windows 애플리케이션에서의 정상 동작을 보장하는 것은 아닙니다.**

CTest를 다시 실행하려면 빌드 구성 후 다음 명령을 사용할 수 있습니다.

```powershell
ctest --test-dir build -C Release --output-on-failure
```

---

## English Guide

### Overview

**French Accent Input** is a Windows 11 x64 utility that types French accented characters without changing the current keyboard layout.

Hold **Left Alt** and press a supported letter (`a`, `c`, `e`, `i`, `o`, `u`, or `y`).

Caps Lock and Shift determine whether the inserted accent is lowercase or uppercase.

The application runs in the Windows notification area (system tray).

### Quick Install

1. Visit the **[latest GitHub Release](https://github.com/Jesuisben/FrenchAccentInput/releases/latest)**.
2. Download `FrenchAccentInput-Setup-1.1.1.exe` from **Assets**.
3. Run the installer and follow its setup wizard.
4. Launch **French Accent Input** after installation.

You can also download `FrenchAccentInput.exe` to run the standalone version without installing it.

`SHA256SUMS.txt` contains file hashes for download verification.

**Note:** The executables are not code-signed, so Windows SmartScreen may display a warning.

Verify the source of any downloaded file before deciding whether to run it.

### Supported Shortcuts

| Left Alt + key | Lowercase cycle | Uppercase cycle |
| --- | --- | --- |
| `a` | à → â → æ | À → Â → Æ |
| `c` | ç | Ç |
| `e` | é → è → ê → ë | É → È → Ê → Ë |
| `i` | ï → î | Ï → Î |
| `o` | ô → œ | Ô → Œ |
| `u` | ù → û → ü | Ù → Û → Ü |
| `y` | ÿ | Ÿ |

Repeated presses of keys with multiple accents cycle through the alternatives by replacing the previous character when supported.

With Caps Lock off, **Left Alt** inserts lowercase accents and **Shift + Left Alt** inserts uppercase accents; Caps Lock reverses these cases.

Right Alt/AltGr, Ctrl, Windows-key shortcuts, and unsupported combinations are passed to the original application.

Korean IME `A` mode permits accents, while `가` mode passes keys through to preserve Hangul composition.

### Installation and Operation

The Inno Setup installer is configured for per-user installation, with optional shortcuts and sign-in launch settings.

The default installation path is `%LOCALAPPDATA%\Programs\FrenchAccentInput`.

Closing the usage window does not stop the app; use the tray icon's **Exit** menu to quit.

Uninstall the program through Windows **Installed apps**.

### Build and Testing

The application uses C++20, Win32 API, Windows IME/IMM, CMake/CTest, MSVC, and Inno Setup.

In PowerShell at the repository root, run:

```powershell
.\scripts\build-release.ps1
```

The release script builds the app, runs CTest, builds the installer, and generates SHA-256 checksums in `dist/`.

The documented v1.1.1 build passed a warnings-as-errors Release build, CTest (2/2), and installer compilation on Windows 11 Pro x64 build 26200.

Automated tests and synthetic input checks do not guarantee physical-keyboard behavior or compatibility with every application.

### Limitations and Privacy

Elevated applications, secure desktops, anti-cheat games, remote sessions, and custom input controls are not guaranteed to work.

The current source does not implement telemetry, network requests, account sign-in, or storage of typed text and window titles.

See **[Troubleshooting](docs/TROUBLESHOOTING.md)** for user checks and known limitations.

---

## License

The source code and documentation are released under the **[MIT License](LICENSE)**.

Inno Setup and Microsoft development or runtime tools are subject to their own license terms.
