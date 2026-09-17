# FrenchAccentInput 진행 기록

마지막 갱신: 2026-09-18

## 2026-09-18 현재 증거 — 이전 원인 확정 문구보다 우선

- 시작 기준선: CMakeLists.txt와 scripts/build-release.ps1 수정, tests/win32_output_tests.cpp untracked 상태였다. 기존 변경을 유지했다.
- 시작 source에서 Release build, CTest 2/2, installer compile을 실행했다. 이것은 이후 수정한 source의 최종 검증이 아니다.
- 실제 메모장 + 별도 virtual hook host + Computer Use `Alt_L+e`에서 é와 menu access-key UI가 함께 표시됐다. 고정 synthetic driver의 EEEE도 같은 menu UI를 재현했다. driver는 전송 성공, 대상 HWND 유지, Alt/Ctrl/Shift 해제를 확인했다.
- 자체 marker의 고정 synthetic event만 계측해 E down/up이 hook에서 suppress되고 output VK_PACKET이 전달되는 것을 확인했다. 일반 사용자 입력·창 제목은 로그에 저장하지 않았다.
- Ctrl로 restored Alt DOWN까지 감싸는 대조 실험도 실패했다. initial/restored Alt DOWN을 보류하는 진단에서는 menu UI가 사라졌지만 EEEE가 4글자 추가됐다. masked Alt UP을 유지해도 Unicode U+0008은 삭제되지 않았다.
- `VK_BACK` down/up으로 바꾸자 EEEE가 한 글자 ë로 교체됐다. output test는 변경 전 2개 assertion FAIL, 수정 후 PASS였다.
- Ctrl 대신 unassigned non-modifier VK 0xE8을 mask로 사용하면 기존 Alt 전환 구조에서도 menu UI가 사라졌다. 해당 output regression은 변경 전 6개 assertion FAIL, 수정 후 PASS였다. 대규모 router 재작성은 하지 않았다.
- 수정 production source로 build한 virtual host에서 mapping sequence AAACCCEEEEIIOOUUUYYY를 입력했다. 메모장 총 글자 수가 11에서 22로 증가했고 추가 부분은 æçççëîœüÿÿÿ였다. menu UI가 없고 driver의 modifier release 검사도 PASS했다. clean field 및 다른 앱 재검증은 진행 중이다.
- 이 증거는 synthetic hook integration과 화면 관찰이다. 실제 물리 keyboard, typematic, Korean IME PASS는 아직 없다. 아래 2026-09-17의 원인 확정/해결 문구는 당시 가설과 이전 source 기록이며 최종 해결 증거가 아니다.
- 전체 fresh 검증, partial output 복구·stale replacement 점검이 남아 있다. 현재 CODEX 단계이며 사용자 작업은 없다.

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
- [x] 영어 우선·한국어 후순서 README 작성
- [x] 운영 관제 instructions 시스템 작성
- [ ] 실제 물리 Left Alt failure root cause 조사·수정·fresh 자동 검증
- [ ] 물리 keyboard, 한국어 IME, 대표 앱의 target-visible text 재확인
- [ ] FINAL GUIDE GATE 충족 후 한국어 완전 제작 가이드 fresh 작성

## 구현된 동작

- 왼쪽 Alt + `A/C/E/I/O/U/Y`만 처리한다.
- 순환표: `A → à â æ`, `C → ç`, `E → é è ê ë`, `I → ï î`, `O → ô œ`, `U → ù û ü`, `Y → ÿ`.
- 같은 Alt session·같은 key·같은 foreground target에서만 직전 문자를 교체한다.
- `Alt+Tab`, `Alt+F4`, Ctrl/Shift/Windows 조합, 오른쪽 Alt/AltGr, 지원하지 않는 key는 통과시킨다.
- injected input은 통과시켜 재귀를 막는다.
- mouse click·wheel, target 변경, 다른 key, Alt 해제 시 교체 상태를 취소한다.
- 일반 사용자 권한, system tray 종료, 중복 실행 방지, 입력 실패 시 fail-open을 제공한다.
- network, telemetry, key/text/window-title 저장이 없다.

## fresh 자동 검증 근거와 한계

- 2026-09-17 수정 source에서 Visual Studio Build Tools 18, CMake 4.2.3, MSVC 19.50, Windows SDK 10.0.26100 warning-as-error Release x64 build 성공.
- 새 regression test를 추가했다. 수정 전 `Left Alt release after an accent must not activate the app menu` assertion이 실패했고, 수정 후 CTest `input_core` 1/1 통과, 실패 0.
- standalone lifecycle: 첫 instance exit `0`, 중복 실행 exit `0`, `--quit-existing` exit `0`.
- installer: Inno Setup 7.1.0 compile 성공.
- fresh silent install 후 `French Accent Input 1.0.0`, version `1.0.0`, 설치 EXE 확인.
- 설치본 실행·종료 exit `0`; 제거 exit `0`; 설치 folder와 uninstall registry 항목 제거 확인.
- Markdown code fence 짝과 공개 source의 개인 식별자·email·credential 후보를 정적 검사한다.

위 자동 검증은 2026-09-17 기존 source의 기록이다. 이후 보고된 실제 물리 Left Alt failure를 해결하거나 증명하지 않는다.

## 현재 산출물

- `dist\FrenchAccentInput.exe` — 22,016 bytes
- `dist\FrenchAccentInput-Setup-1.0.0.exe` — 2,110,620 bytes
- `dist\SHA256SUMS.txt`
  - installer: `8b1ba422abba3163d955ffb9ff18b66c13371d8b03cd9e3f49035b154bb68954`
  - standalone: `7a00bb4512f50ea7f85955bcfad9a787942f5a1a242099ccdbc451cd42760d71`

## 현재 Release blocker

Windows 11 실제 물리 Left Alt를 누른 채 E를 입력할 때 다음 간헐 FAIL이 보고됐다.

- `é` 입력은 될 때와 안 될 때가 있다.
- Alt만 누를 때의 menu/access-key UI가 accent 입력 중 간헐적으로 활성화된다.
- 같은 Alt session에서 E 반복 시 `é → è → ê → ë`로 직전 한 글자를 교체해야 하지만, replacement/cycle이 간헐적으로 실패한다.
- 같은 환경에서 Alt+Tab은 정상이다. 물리 Alt 고장으로 단정하지 않는다.

기존 synthetic `VK_LMENU` key-up/down 경로는 menu/access-key UI와 충돌할 가능성이 있었지만, 아직 root cause로 확정되지 않았다. Windows `DefWindowProc`은 Alt `WM_SYSKEYUP`에 `SC_KEYMENU`를 보낼 수 있다. 반대로 Microsoft 문서상 `SendInput`은 이미 눌린 modifier state를 초기화하지 않으므로, synthetic Alt transition을 전부 제거한 현재 경로는 Unicode 입력을 방해할 수 있다. 이 두 가설을 virtual integration으로 비교 중이다.

accent 뒤 physical Left Alt key-up suppress는 별도 원인으로 확인됐다. hook에서 nonzero를 반환하면 target이 key-up을 받지 못해 modifier release가 불완전해진다. 이 suppress는 제거했고 RED/GREEN unit test를 확인했다. replacement Backspace는 Unicode `\b` input으로 보낸다.

이 수정의 물리 재검증에서 accent 뒤 UI button click이 비정상이라는 FAIL이 보고됐다. root cause는 `InputRouter`가 accent 뒤 physical Left Alt key-up을 suppress한 것이다. hook에서 nonzero를 반환하면 target이 key-up을 받지 못하므로 OS modifier release가 불완전해진다. 이 suppress를 제거했고 RED/GREEN unit test를 확인했다.

수정 source에서 fresh Release x64 build, CTest 1/1, standalone lifecycle, silent installer install/run/exit/uninstall, 설치 folder·registry 제거를 성공했다. 이전 계산기 button click은 accent keyboard path를 실행하지 않았으므로 이번 bug의 증거로 사용하지 않는다. virtual keyboard가 실제 hook·output·foreground target text 경로를 실행하는 integration test를 추가한 뒤에만 사용자 physical validation을 다시 요청한다.

현재 output 경로는 Ctrl-mask + temporary Left Alt up → Unicode (필요 시 Backspace 포함) → Left Alt down으로 한 가지다. accent가 실제 처리된 session의 마지막 physical Left Alt up은 Ctrl-mask + synthetic Alt up으로 대체 전달하며, output injection이 실패하면 fail-open으로 원래 key-up을 통과시킨다. 이 regression rule은 CTest로 확인했고, 이후 Release build·standalone lifecycle·installer install/run/uninstall도 2026-09-17 fresh PASS했다. target-visible virtual keyboard verification은 foreground를 가로채는 화상 키보드 상태를 분리한 뒤 계속 진행한다.

## 남은 실제 사용자 확인

software가 만든 key는 제품이 의도적으로 무시하므로 자동화 입력은 물리 keyboard의 성공 근거가 아니다. 수정 후 `instructions/02_USER_ACTION_REQUIRED.md`의 최소 체크리스트만 수행한다.

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

`C:\myDdrive\FrenchAccentInput\instructions\00_READ_FIRST.md를 fresh하게 전체 읽고, 그 파일이 지시하는 현재 단계 문서와 source of truth를 순서대로 읽어. 현재 담당이 CODEX라면 불필요한 수동 작업을 요구하지 말고 검증 가능한 범위까지 자동으로 진행해. Git/GitHub 변경은 하지 마.`
