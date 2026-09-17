# 트러블슈팅 기록

## 기록 원칙

문제의 증상, 직접 원인, 확인 근거, 수정, 재검증을 순서대로 남긴다. 예상과 실제 실행 결과를 섞지 않는다.

## 2026-09-18: 실제 메모장 synthetic 경로에서 두 failure 분리

- 직접 관찰: production과 같은 source의 virtual host에서 Alt+E 후 é와 access-key UI가 함께 나타났다. 고정 EEEE driver도 재현했다. 전송 성공, foreground HWND 유지, modifier 해제 여부를 따로 확인했다.
- hook 경계: driver/output 자체 marker에 한정한 계측에서 E 이벤트 suppress와 VK_PACKET 출력을 확인했다. 일반 입력은 기록하지 않았다.
- menu 원인 범위: Ctrl-mask만으로 Windows 11 메모장의 access-key UI를 취소하지 못했다. Ctrl로 Alt DOWN까지 감싸도 실패했다. Alt DOWN을 보류하는 진단에서는 메뉴가 사라졌지만 native shortcut 재전달이 필요하므로 제품에 채택하지 않았다.
- 최소 수정: 기존 Alt 전환을 유지하고 Ctrl 대신 unassigned VK 0xE8 non-modifier로 mask했다. 동일 메모장 고정 synthetic EEEE에서 menu UI 없이 ë 한 글자가 추가됐다. 이는 이 환경의 synthetic 재현에 대한 근거이며 실제 physical failure 전체의 해결을 단정하지 않는다.
- replacement 원인: Unicode U+0008/VK_PACKET은 이 메모장에서 실제 Backspace 삭제가 되지 않아 EEEE가 4글자로 누적됐다. masked Alt UP을 유지한 대조에서도 동일했다. `VK_BACK` down/up으로 수정 후 한 글자로 순환했다.
- regression: 실제 Backspace 계약은 변경 전 2개 assertion FAIL 후 GREEN, non-modifier mask 계약은 변경 전 6개 assertion FAIL 후 GREEN. 기존 input core는 유지했다.
- 근거 문서: [Microsoft Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes), [AutoHotkey 공식 menu mask 설명](https://github.com/AutoHotkey/AutoHotkeyDocs/blob/v2/docs/lib/A_MenuMaskKey.htm). 0xE8은 현재 unassigned이며 향후 Windows 변경 시 재검증한다.
- 아래 2026-09-17의 Ctrl-mask root-cause/해결 기록은 이전 source의 가설과 검증 범위다. 최신 재현이 이를 supersede한다. 최종 Release·installer·대표 앱·물리 검증은 아직 완료하지 않았다.

## 2026-09-17: 실제 물리 Left Alt accent 입력이 간헐적으로 실패함

- 증상: Windows 11 실제 물리 Left Alt를 누른 채 E를 입력하면 `é`가 입력될 때와 아닐 때가 있고, accent 입력 중 menu/access-key UI가 간헐적으로 활성화됐다. 같은 Alt session의 E 반복도 직전 문자를 교체하지 못하는 경우가 있었다.
- 재현 조건: 실제 물리 Left Alt hold + E 반복. Alt+Tab은 같은 환경에서 정상 동작했다.
- 조사 과정: `WH_KEYBOARD_LL`이 `WM_KEYDOWN`/`WM_SYSKEYDOWN`/`WM_KEYUP`/`WM_SYSKEYUP`을 받는 흐름과 Alt+Tab pass-through를 비교했다. Microsoft [LowLevelKeyboardProc](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc), [KBDLLHOOKSTRUCT](https://learn.microsoft.com/ko-kr/windows/win32/api/winuser/ns-winuser-kbdllhookstruct), [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput), [KEYBDINPUT](https://learn.microsoft.com/ko-kr/windows/win32/api/winuser/ns-winuser-keybdinput), [WM_SYSKEYUP](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-syskeyup) 문서와 대조했다. `LLKHF_ALTDOWN`은 Alt context code이고 `SendInput`은 현재 keyboard state를 reset하지 않아 이미 눌린 key가 input에 간섭할 수 있다. `KEYEVENTF_UNICODE`는 `VK_PACKET` 뒤 `WM_CHAR`를 만든다.
- root cause: `SendInput`은 이미 눌린 modifier state를 reset하지 않는다. 그래서 physical Left Alt를 계속 누른 채 Unicode만 주입하면 입력이 방해될 수 있다. 반대로 plain synthetic Alt up/down은 `WM_SYSKEYUP`과 `SC_KEYMENU` 경로를 만들 수 있다. physical Alt key-up 자체를 단순 suppress하는 것은 해결책이 아니며 다음 항목의 UI click regression을 만들었다.
- 수정: output 때 Ctrl-mask + temporary Left Alt up → Unicode `\b`/character → Left Alt down을 보낸다. 실제 accent session의 마지막 physical Left Alt up은 Ctrl-mask + synthetic Alt up으로 대체 전달한다. injection 실패 시 원래 key-up은 fail-open으로 통과한다.
- RED/GREEN: `accent_masks_following_left_alt_release`와 failed-output Alt release test를 추가했다. 이전 단순 suppress 구현은 `Left Alt release after an accent must reach Windows to clear modifier state`로 실패했고, 대체 release routing 후 CTest가 통과했다.
- 자동 재검증: warning-as-error Release x64 build, standalone first/duplicate/`--quit-existing`, Inno Setup 7.1.0 build, silent install, 설치 EXE, 설치본 run/exit, uninstall, install folder와 uninstall registry 제거를 fresh하게 성공했다.
- 남은 사용자 물리 검증: `instructions/02_USER_ACTION_REQUIRED.md`의 Left Alt/menu UI, representative app, Korean IME checklist가 PASS해야 한다.

## 2026-09-17: accent 뒤 UI button click 비정상

- 증상: 사용자 물리 재검증에서 French Accent Input 사용 뒤 버튼이 클릭되지 않는 비정상 상태가 보고됐다.
- 재현 정보: 앱·키·반복 조건은 아직 상세 미확정이다. 즉시 `02_USER_ACTION_REQUIRED.md`를 사용자 작업 없음으로 되돌리고 Codex가 조사한다.
- root cause: `InputRouter::suppress_left_alt_release_`가 accent 뒤 physical Left Alt key-up을 hook에서 막았다. `WH_KEYBOARD_LL` callback이 nonzero를 반환하면 system이 event를 target window procedure로 전달하지 않는다. 따라서 target은 modifier release를 받지 못해 button click 등 후속 UI input이 비정상일 수 있다.
- RED/GREEN: test를 `accent_preserves_following_left_alt_release`로 바꿨다. 수정 전 `Left Alt release after an accent must reach Windows to clear modifier state` assertion이 실패했고, `suppress_left_alt_release_`와 해당 suppress를 제거한 뒤 CTest가 통과했다.
- 자동 재검증: 수정 source Release x64 build, CTest 1/1, standalone first/duplicate/`--quit-existing`, silent installer install/run/exit/uninstall, install folder·uninstall registry 제거를 fresh하게 성공했다.
- 폐기한 검증: Computer Use 계산기 button click은 accent keyboard path를 실행하지 않았다. 따라서 이 failure의 root-cause 또는 fix 증거가 아니며 release evidence에서 제외한다.
- 다음 자동 검증: test-only hook host가 virtual keyboard input을 routing하고 real foreground target의 visible text를 확인한다. physical keyboard와 IME PASS는 여전히 별도 evidence다.

## 2026-09-17: toolchain 명령이 PATH에 없음

- 증상: `cmake`, `ctest`, `cl` 명령을 일반 PowerShell에서 찾지 못했다.
- 원인: Visual Studio Build Tools의 bundled CMake가 설치되어 있지만 PATH에 등록되지 않았다.
- 확인: `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`와 Visual Studio 18 2026 generator를 확인했다.
- 해결: build script에서 bundled CMake 절대 경로를 찾은 뒤 사용한다.
- 재검증: build script의 configure·Release build·CTest가 모두 성공했다.

## 2026-09-17: test가 `nodiscard` 경고로 build 실패

- 증상: test에서 상태 준비용 `router.handle(...)` 반환값을 버리자 MSVC `/WX`가 경고를 오류로 바꿨다.
- 원인: `handle`은 실수로 routing 결과를 무시하지 않도록 `[[nodiscard]]`로 선언되어 있다.
- 확인: compiler가 반환값을 사용하지 않은 각 줄을 정확히 가리켰다.
- 해결: 의도적으로 결과를 버리는 준비 단계에 `(void)`를 명시했다. production의 안전 속성은 제거하지 않았다.
- 재검증: test target build와 CTest 1/1이 통과했다.

## 2026-09-17: `WIN32_LEAN_AND_MEAN` 중복 정의

- 증상: Release build가 macro 재정의 경고를 냈고 `/WX` 때문에 실패했다.
- 원인: 같은 macro를 CMake compile definition과 `main.cpp`에서 모두 정의했다.
- 확인: compiler command와 source 첫 줄에서 중복을 확인했다.
- 해결: build 설정을 단일 책임 지점으로 삼고 source의 중복 정의를 삭제했다.
- 재검증: warning-as-error Release build가 성공했다.

## 2026-09-17: manifest resource 중복

- 증상: link 단계에서 `CVT1100`과 `LNK1123` 오류가 발생했다.
- 원인: MSVC linker가 만드는 기본 manifest와 별도 `RT_MANIFEST` resource가 같은 ID를 사용했다.
- 확인: custom manifest를 제외한 build에서 link가 통과했다.
- 해결: 별도 manifest를 제거하고 CMake/MSVC의 기본 `asInvoker` manifest를 사용했다.
- 재검증: GUI EXE Release link와 process smoke test가 성공했다.

## 2026-09-17: 설치 검증의 제품명 정확 일치가 실패

- 증상: silent 설치는 성공했지만 검증 script가 uninstall registry 항목을 찾지 못했다고 판단했다.
- 원인: installer의 실제 `DisplayName`은 `French Accent Input 1.0.0`인데 검증식이 `French Accent Input`과 정확히 같은 값만 찾았다.
- 확인: `HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\{4A67FCE1-5DC0-4ACB-9843-9672E4CBE071}_is1`을 읽어 실제 이름과 version을 확인했다.
- 해결: 제품 계보를 고정하는 `AppId`는 유지하고 검증식만 `French Accent Input*`으로 수정했다.
- 재검증: 설치 파일, registry version, 실행·종료, uninstaller exit code, 설치 폴더와 registry 제거를 모두 확인했다.
