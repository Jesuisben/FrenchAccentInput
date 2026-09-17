# CODEX CURRENT TASK

## 목표

actual hook과 virtual Left Alt keyboard event가 foreground target에 만드는 text를 자동 검증한다. 이 integration test는 배포 기능이나 사용자 설정이 아니며, physical keyboard PASS를 대체하지 않는다.

## 지금 할 일

1. production EXE와 분리된 test-only hook host로, Alt가 눌린 상태의 Unicode injection과 temporary Alt bridge를 비교한다.
2. virtual event가 hook을 통과하는지와 foreground target text를 Computer Use로 직접 읽는다. foreground를 가로채는 도구는 test evidence에서 분리한다.
3. root cause를 확정하기 전에는 production output strategy를 바꾸지 않는다.
4. RED/GREEN, CTest, Release, lifecycle, installer를 fresh하게 다시 실행한다.
5. 사용자에게는 자동 검증을 모두 끝낼 때까지 물리 테스트를 요청하지 않는다.

## 고정 경계

- `docs/BUILD_GUIDE.ko.md`는 final guide gate 전까지 생성하지 않는다.
- Git/GitHub 변경을 하지 않는다.
- 물리 keyboard, 실제 Alt/menu UI, 한국어 IME, 대표 앱 target-visible text는 사용자만 PASS할 수 있다.
- Alt+Tab, Alt+F4, Ctrl shortcut, Shift, Windows key, Right Alt/AltGr, unsupported Alt shortcut은 통과해야 한다.

## 작업 지속 규칙

- 한 test 또는 build가 PASS해도 final 보고로 작업을 끝내지 않는다. 이 문서의 다음 자동 항목을 바로 실행한다.
- physical validation은 모든 자동 항목이 실제 명령 output으로 끝난 뒤의 마지막 gate다. 그 전에는 사용자에게 요청하거나 blocker로 선언하지 않는다.
- 보고가 필요하면 commentary로 짧게 현재 증거와 다음 자동 작업을 알린 뒤 계속한다.
