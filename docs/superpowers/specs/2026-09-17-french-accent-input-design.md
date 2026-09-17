# French Accent Input 설계 명세

## 1. 목표

Windows 11의 기존 QWERTY·한국어 입력 환경을 바꾸지 않고, 왼쪽 Alt와 프랑스어 알파벳 키 조합으로 악상 문자를 모든 일반 데스크톱 앱에 입력하는 무료 오픈소스 도구를 만든다.

## 2. 직접 확인한 TypeIt 동작

2026-09-17에 `https://french.typeit.org/`에서 다음 동작을 직접 확인했다.

- Alt를 유지한 채 `E` 1~5회: `é`, `è`, `ê`, `ë`, 다시 `é`
- Alt를 유지한 채 `E`, `A`: `éà`
- Alt를 유지한 채 `C` 3회: `ççç`
- 화면의 단축키 안내: `A → à â æ`, `C → ç`, `E → é è ê ë`, `I → ï î`, `O → ô œ`, `U → ù û ü`, `Y → ÿ`

TypeIt의 소스 코드는 사용하지 않는다. 공개된 사용자 동작만 독자적으로 재현한다.

## 3. 확정 범위

- Windows 11 x64용 native GUI 프로그램
- 왼쪽 Alt만 조합 시작 키로 사용
- `A/C/E/I/O/U/Y`의 소문자 악상 입력
- 여러 문자가 있는 키는 같은 Alt 세션에서 교체·순환
- `C`, `Y`는 누를 때마다 새 문자 추가
- 정확히 처리하는 조합은 원래 앱 단축키보다 이 프로그램이 우선
- `Alt+Tab`, `Alt+F4`, Ctrl 단축키, Windows 키, 오른쪽 Alt/AltGr는 기존 동작 유지
- system tray에서 실행 상태 확인과 종료
- 같은 사용자 세션에서 중복 실행 방지
- 일반 사용자 권한 설치와 제거
- 입력 내용, 활성 창 이름, 키 기록, telemetry, network 통신 없음

## 4. 비범위

- 대문자 악상, 문장부호, 설정 UI, 자동 업데이트
- 관리자 권한 앱, 보안 데스크톱, 게임 anti-cheat, 원격 세션 호환 보장
- 유료 code signing과 SmartScreen 무경고 보장
- Git commit, push, tag, GitHub Release 생성

## 5. 접근 비교와 선택

1. Python: 배우기 쉽지만 runtime bundle과 tray dependency가 크고 low-level hook 경계가 흐려진다.
2. AutoHotkey: 코드가 짧지만 installer·오픈소스 검토·정밀한 injected-event 제어에 불리하다.
3. C++20 + Win32: 추가 runtime 없이 단일 EXE를 만들고 `WH_KEYBOARD_LL`, `SendInput`, tray, mutex를 직접 제어한다.

선택은 3번이다. 이 프로젝트에서 Windows 전용 native API는 dependency를 늘리지 않는 가장 작은 해법이다.

## 6. 구조

- `input_core`: OS와 분리된 상태 머신과 키 routing. 자동 테스트 대상이다.
- `main.cpp`: keyboard/mouse hook, Unicode 전송, tray, message loop, single-instance mutex를 담당한다.
- `CMakeLists.txt`: app과 test를 동일한 compiler 설정으로 build한다.
- `installer/FrenchAccentInput.iss`: Inno Setup per-user installer를 만든다.

입력 흐름:

1. `WH_KEYBOARD_LL` callback이 실제 키 event를 받는다.
2. 자체 주입 또는 다른 injected event는 통과시킨다.
3. 순수 router가 modifier와 지원 키를 판정한다.
4. 처리 대상이면 원래 문자 event를 억제한다.
5. 왼쪽 Alt를 잠시 올리고, 필요하면 Backspace를 보낸 뒤 Unicode 문자를 보낸다.
6. 실제 Alt가 계속 눌린 상태면 왼쪽 Alt를 다시 내려 native shortcut 상태를 보존한다.
7. 자체 event는 `dwExtraInfo` marker로 재처리하지 않는다.

## 7. 안전 규칙

- hook callback은 routing과 짧은 `SendInput`만 수행한다.
- 지원하지 않는 event는 반드시 `CallNextHookEx`로 넘긴다.
- foreground window가 바뀌거나 mouse/다른 키가 개입하면 교체 상태를 취소한다.
- `SendInput`이 0개를 보냈으면 원래 event를 통과시켜 fail-open한다.
- 부분 전송은 상태를 취소하고 tray 알림을 남긴다.
- UIPI 때문에 더 높은 권한 앱에는 입력이 막힐 수 있음을 문서화한다.

## 8. 검증

- 자동: TypeIt 문자 순서, 순환, 단일 문자 반복, target 변경, modifier 충돌, injected event, keyup 억제
- build: Release x64 app과 test compile, CTest
- package: Inno Setup compile, installer 파일 hash
- 실제 앱: 메모장·브라우저에서 입력, `Alt+Tab`, `Alt+F4`, Ctrl 단축키, 오른쪽 Alt, 한국어 IME
- installer: 새 설치, 실행, Installed Apps 등록, 제거

자동화된 synthetic input은 물리 키보드와 동일한 증거로 취급하지 않는다. 물리 키보드 검증이 남으면 `progress.md`에 구분해서 기록한다.

