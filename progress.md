# FrenchAccentInput 진행 기록

마지막 갱신: 2026-09-17

## 현재 목표

TypeIt 방식의 왼쪽 Alt 프랑스어 악상 입력을 Windows 11 전체에서 제공하는 native EXE와 일반 사용자용 installer를 만든다.

## 현재 단계

- [x] 사용자 요구사항과 비범위 정리
- [x] TypeIt 실제 shortcut·순환 동작 확인
- [x] C++20 + Win32 설계 명세와 구현 계획 작성
- [x] 순수 입력 core를 test-first로 구현
- [x] Win32 keyboard/mouse hook, Unicode 입력, tray, 단일 instance 구현
- [x] Release x64 build, CTest, process lifecycle 검증
- [x] per-user installer build·설치·실행·제거 검증
- [x] 영어 우선·한국어 후순서 README와 한국어 완전 제작 가이드 작성
- [x] 실제 문제와 해결을 `docs/TROUBLESHOOTING.md`에 기록
- [ ] 물리 keyboard, 한국어 IME, 대표 앱의 target-visible text 확인

## 구현된 동작

- 왼쪽 Alt + `A/C/E/I/O/U/Y`만 처리한다.
- 순환표: `A → à â æ`, `C → ç`, `E → é è ê ë`, `I → ï î`, `O → ô œ`, `U → ù û ü`, `Y → ÿ`.
- 같은 Alt session·같은 key·같은 foreground target에서만 직전 문자를 교체한다.
- `Alt+Tab`, `Alt+F4`, Ctrl/Shift/Windows 조합, 오른쪽 Alt/AltGr, 지원하지 않는 key는 통과시킨다.
- injected input은 통과시켜 재귀를 막는다.
- mouse click·wheel, target 변경, 다른 key, Alt 해제 시 교체 상태를 취소한다.
- 일반 사용자 권한, system tray 종료, 중복 실행 방지, 입력 실패 시 fail-open을 제공한다.
- network, telemetry, key/text/window-title 저장이 없다.

## 현재 검증 근거

- Visual Studio Build Tools 18, CMake 4.2.3, MSVC 19.50, Windows SDK 10.0.26100에서 warning-as-error Release build 성공.
- CTest: `input_core` 1/1 통과, 실패 0.
- standalone lifecycle: 첫 instance exit `0`, 중복 실행 exit `0`, `--quit-existing` exit `0`.
- installer: Inno Setup 7.1.0 compile 성공.
- silent 설치 후 `French Accent Input 1.0.0`, version `1.0.0`, 설치 EXE 확인.
- 설치본 실행·종료 exit `0`; 제거 exit `0`; 설치 folder와 uninstall registry 항목 제거 확인.
- 가이드의 source code block 10개와 실제 file 비교 결과 불일치 없음.
- Markdown code fence 짝과 공개 source의 개인 식별자·email·credential 후보를 정적 검사한다.

## 현재 산출물

- `dist\FrenchAccentInput.exe` — 20,480 bytes
- `dist\FrenchAccentInput-Setup-1.0.0.exe` — 2,109,945 bytes
- `dist\SHA256SUMS.txt`
  - installer: `c23a4c21d7a8cdf8b6a78c7d4d234a8b08205138b2b14e462902bdae0e16c2aa`
  - standalone: `93cb92b411990dc781346092b951912b12999d43db77e4c74f3de009596d3e97`

## 남은 실제 사용자 확인

software가 만든 key는 제품이 의도적으로 무시하므로 자동화 입력은 물리 keyboard의 성공 근거가 아니다. Release 전에 `docs/BUILD_GUIDE.ko.md` 16절의 한 묶음만 수행한다.

1. 메모장에서 전체 mapping·순환·문자 전환을 확인한다.
2. `Alt+Tab`, `Alt+F4`, Ctrl shortcut, 오른쪽 Alt/AltGr를 확인한다.
3. 한국어 IME와 browser·Word 또는 VS Code에서 보이는 text를 확인한다.
4. stale replacement가 한 번이라도 보이면 Release를 중단하고 정확한 재현을 기록한다.

## 고정 경계

- 작업 root: `C:\myDdrive\FrenchAccentInput`
- `Study_LLM_Wiki_Public`는 읽기만 하며 이번 작업에서 수정하지 않았다.
- Git commit, push, tag, remote, GitHub Release는 변경하지 않았다. 이후에도 사용자가 직접 수행한다.
- README는 영어가 먼저이고 한국어가 아래에 있다.
- 자동 검증, 정적 검증, 물리 확인을 서로 대신했다고 주장하지 않는다.

## 새 채팅 시작 문구

`C:\myDdrive\FrenchAccentInput\progress.md를 읽고, Git/GitHub를 변경하지 말고 다음 미완료 작업부터 자동으로 계속해. 중간 질문 없이 검증 가능한 범위까지 진행해.`
