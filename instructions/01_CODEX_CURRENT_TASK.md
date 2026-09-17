# CODEX CURRENT TASK

## 목표

actual hook과 virtual Left Alt keyboard event가 foreground target에 만드는 text를 자동 검증한다. 이 integration test는 배포 기능이나 사용자 설정이 아니며, physical keyboard PASS를 대체하지 않는다.

## 지금 할 일

1. `src/main.cpp`는 VK 0xE8 mask와 실제 VK_BACK으로 수정됐다. 두 변경 각각 output test RED/GREEN과 메모장 synthetic GUI 차이를 확인했다. Ctrl-mask/Unicode Backspace로 되돌리지 않는다.
2. `tests/virtual_keyboard_driver.cpp`는 지정 HWND가 foreground일 때만 고정 synthetic 입력을 보낸다. `e-cycle`은 ë 한 글자, `mapping`은 æçççëîœüÿÿÿ를 추가해야 한다. production과 같은 source의 `FrenchAccentInputVirtualKeyboardHost`를 유지한 상태에서 실행하고 Computer Use로 최종 표시를 읽는다. 이 host는 injected event 허용 차이가 있으므로 물리 PASS가 아니다.
3. 메모장 clean field, browser, VS Code에서 mapping/cycle/일반 입력/UI 복귀를 추가 검증한다. partial SendInput 복구, focus/caret 변경, injected 외부 입력 이후 stale replacement, shortcut 비간섭의 source/test 누락을 점검하고 regression으로 보완한다.
4. 최종 수정 뒤 warning-as-error Release, 전체 CTest, standalone lifecycle/duplicate/quit, installer install/run/exit/uninstall, 설치 폴더·registry 정리, SHA-256을 모두 fresh 검증한다.
5. 알려진 자동 failure가 없어야 물리 체크리스트를 작성한다. 현재 사용자 작업은 없다.

## 고정 경계

- `docs/BUILD_GUIDE.ko.md`는 final guide gate 전까지 생성하지 않는다.
- Git/GitHub 변경을 하지 않는다.
- 물리 keyboard, 실제 Alt/menu UI, 한국어 IME, 대표 앱 target-visible text는 사용자만 PASS할 수 있다.
- Alt+Tab, Alt+F4, Ctrl shortcut, Shift, Windows key, Right Alt/AltGr, unsupported Alt shortcut은 통과해야 한다.

## 작업 지속 규칙

- 한 test 또는 build가 PASS해도 final 보고로 작업을 끝내지 않는다. 이 문서의 다음 자동 항목을 바로 실행한다.
- physical validation은 모든 자동 항목이 실제 명령 output으로 끝난 뒤의 마지막 gate다. 그 전에는 사용자에게 요청하거나 blocker로 선언하지 않는다.
- 보고가 필요하면 commentary로 짧게 현재 증거와 다음 자동 작업을 알린 뒤 계속한다.
