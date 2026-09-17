> **English first. 한국어 안내는 아래에 있습니다.**

# French Accent Input

French Accent Input adds TypeIt-style French accent shortcuts to ordinary Windows applications without changing your keyboard layout.

Hold **Left Alt** and press a supported letter:

| Keys while Left Alt stays down | Output |
|---|---|
| `A`, `AA`, `AAA` | `à`, `â`, `æ` |
| `C` | `ç` |
| `E`, `EE`, `EEE`, `EEEE` | `é`, `è`, `ê`, `ë` |
| `I`, `II` | `ï`, `î` |
| `O`, `OO` | `ô`, `œ` |
| `U`, `UU`, `UUU` | `ù`, `û`, `ü` |
| `Y` | `ÿ` |

Repeated multi-character keys replace and cycle the previous character. `C` and `Y` append one character per press. Exact accent combinations take priority over application shortcuts. `Alt+Tab`, `Alt+F4`, Ctrl shortcuts, the Windows key, and Right Alt/AltGr remain untouched.

## Install

1. Open the repository's latest GitHub Release.
2. Download `FrenchAccentInput-Setup-1.0.0.exe` and `SHA256SUMS.txt`.
3. Verify the installer's SHA-256 value against `SHA256SUMS.txt`.
4. Run the installer. No administrator permission is required.
5. Use the tray icon's **Exit / 종료** command before upgrading or uninstalling.

The unsigned installer can trigger Microsoft Defender SmartScreen. A warning is not proof of malware, but never bypass it without verifying the repository, release, and checksum.

## Privacy and security

- No network access, telemetry, accounts, or updates.
- No keystrokes, text, window titles, or timestamps are stored.
- Only seven fixed letter keys are transformed while Left Alt is held.
- Windows UIPI prevents a normal-permission process from injecting text into elevated applications.

## Build

Windows 11 x64, Visual Studio Build Tools with **Desktop development with C++**, and Inno Setup are required. Inno Setup 7 currently identifies its compiler as non-commercial use only; check its current license before commercial distribution.

```powershell
Set-Location 'C:\path\to\FrenchAccentInput'
.\scripts\build-release.ps1
```

Artifacts are written to `dist\`. See [the Korean build guide](docs/BUILD_GUIDE.ko.md) for the complete source-by-source tutorial, tests, installer verification, GitHub Release steps, and troubleshooting.

## Limitations

- Windows 11 x64 only.
- Lowercase French letters only in version 1.0.0.
- Elevated apps, secure desktops, games with anti-cheat, and remote sessions are not guaranteed.
- Physical keyboard, Korean IME, and representative-app qualification must be checked on the release machine.

This project independently reproduces observed TypeIt interaction. It does not use TypeIt's source code.

---

# French Accent Input 한국어

French Accent Input은 keyboard layout을 바꾸지 않고 일반 Windows 앱에 TypeIt 방식의 프랑스어 악상 단축키를 추가한다.

**왼쪽 Alt**를 누른 상태에서 지원 문자를 누른다.

| 왼쪽 Alt를 유지하며 누르는 키 | 결과 |
|---|---|
| `A`, `AA`, `AAA` | `à`, `â`, `æ` |
| `C` | `ç` |
| `E`, `EE`, `EEE`, `EEEE` | `é`, `è`, `ê`, `ë` |
| `I`, `II` | `ï`, `î` |
| `O`, `OO` | `ô`, `œ` |
| `U`, `UU`, `UUU` | `ù`, `û`, `ü` |
| `Y` | `ÿ` |

여러 문자가 있는 키는 직전 문자를 교체하며 순환한다. `C`, `Y`는 누를 때마다 새 문자를 추가한다. 정확히 처리하는 악상 조합은 앱의 같은 단축키보다 우선한다. `Alt+Tab`, `Alt+F4`, Ctrl 단축키, Windows 키, 오른쪽 Alt/AltGr는 기존대로 동작한다.

## 설치

1. GitHub 저장소의 최신 Release를 연다.
2. `FrenchAccentInput-Setup-1.0.0.exe`와 `SHA256SUMS.txt`를 받는다.
3. installer의 SHA-256 값을 `SHA256SUMS.txt`와 비교한다.
4. installer를 실행한다. 관리자 권한은 필요 없다.
5. upgrade·제거 전 tray icon의 **Exit / 종료**를 누른다.

installer에 code signing이 없으므로 Microsoft Defender SmartScreen 경고가 나타날 수 있다. 저장소·Release·checksum을 확인하지 않은 파일은 실행하지 않는다.

## 개인정보와 보안

- network, telemetry, 계정, 자동 업데이트가 없다.
- 키 입력, 작성한 글, 창 제목, timestamp를 저장하지 않는다.
- 왼쪽 Alt가 눌렸을 때 고정된 7개 문자만 변환한다.
- Windows UIPI 때문에 일반 권한 프로그램은 관리자 권한 앱에 문자를 넣을 수 없다.

## 직접 build

Windows 11 x64, Visual Studio Build Tools의 **Desktop development with C++**, Inno Setup이 필요하다. Inno Setup 7 compiler는 현재 non-commercial use only로 표시되므로 상업 배포 전 당시 license를 다시 확인한다.

```powershell
Set-Location 'C:\path\to\FrenchAccentInput'
.\scripts\build-release.ps1
```

결과물은 `dist\`에 생성된다. 모든 source 설명, test, installer 검증, GitHub Release, 문제 해결은 [한국어 제작 가이드](docs/BUILD_GUIDE.ko.md)에 있다.

## 한계

- Windows 11 x64 전용이다.
- 1.0.0은 프랑스어 소문자만 지원한다.
- 관리자 권한 앱, secure desktop, anti-cheat game, remote session은 보장하지 않는다.
- 최종 Release PC에서 물리 keyboard, 한국어 IME, 대표 앱 검증이 필요하다.

이 프로젝트는 TypeIt에서 관찰한 사용자 동작을 독자적으로 재현한다. TypeIt source code를 사용하지 않는다.

## License

[MIT License](LICENSE)
