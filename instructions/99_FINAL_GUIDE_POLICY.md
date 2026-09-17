# FINAL GUIDE POLICY

## 지금은 공부용 가이드를 만들지 않는다

`docs/BUILD_GUIDE.ko.md`는 final guide가 아니며 이번 작업 시작 시 삭제했다. 버그 수정, 기능 구현, 자동 검증, 사용자 물리 검증 전, 물리 검증 FAIL, unresolved blocker 상태에서는 다시 만들지 않는다.

## FINAL GUIDE GATE

다음이 모두 PASS일 때만 `docs/BUILD_GUIDE.ko.md`를 final source에서 fresh하게 새로 작성한다.

- 기능 구현 최종 완료, unresolved blocker 없음, final source 확정
- fresh regression tests, 전체 CTest, warning-as-error Release build, standalone lifecycle PASS
- installer build, install/run/exit/uninstall PASS
- final artifacts와 checksums 생성
- 사용자 실제 물리 keyboard 검증 완료 및 명시적 PASS
- `progress.md`와 `docs/TROUBLESHOOTING.md`에 final 결과 반영
- 추가 코드 수정 필요 없음

## final guide 범위

빈 Windows 환경에서 초보자가 final source와 일치하는 프로그램을 다시 만들며 공부하는 한국어 완전 제작 교재다. C++20, Win32, keyboard hook, modifier/Alt, Unicode, input architecture, state machine, replacement, injected events, foreground/caret safety, tray, single instance, fail-open, CMake, TDD/tests, Release, installer, artifact/checksum, GitHub Release 절차, 실제 bug와 troubleshooting을 다룬다. 가이드 code와 actual source의 일치도 자동 검증한다.
