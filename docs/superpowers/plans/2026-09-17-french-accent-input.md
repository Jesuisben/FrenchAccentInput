# French Accent Input Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** TypeIt 방식의 프랑스어 악상 입력을 Windows 11 전체에서 제공하는 native EXE와 installer를 만든다.

**Architecture:** 순수 C++ `InputRouter`가 modifier와 순환 상태를 처리한다. Win32 shell은 low-level hooks, `SendInput`, tray, mutex, message loop만 담당한다.

**Tech Stack:** C++20, Win32 API, CMake 4.2, Visual Studio Build Tools, CTest, Inno Setup

**Spec:** `docs/superpowers/specs/2026-09-17-french-accent-input-design.md`

## Global Constraints

- 대상 OS는 Windows 11 x64다.
- 왼쪽 Alt와 `A/C/E/I/O/U/Y`만 소비한다.
- Git과 GitHub 상태를 변경하지 않는다.
- 입력 내용이나 창 제목을 저장·전송하지 않는다.
- `Study_LLM_Wiki_Public`에는 쓰지 않는다.

---

### Task 1: 순수 입력 core

**Files:**
- Create: `include/fai/input_core.h`
- Create: `src/input_core.cpp`
- Create: `tests/input_core_tests.cpp`
- Create: `CMakeLists.txt`

**Interfaces:**
- Produces: `fai::KeyEvent`, `fai::Edit`, `fai::RouteResult`, `fai::InputRouter::handle`, `fai::InputRouter::cancel_sequence`

- [x] `tests/input_core_tests.cpp`에 TypeIt 순환, 문자 전환, 단일 문자 반복, modifier 비간섭, injected-event 통과, target 변경 시험을 먼저 작성한다.
- [x] CMake를 구성하고 test target을 build해 production symbol 부재로 실패함을 확인한다.
- [x] `input_core.h/.cpp`에 최소 상태 머신과 router를 구현한다.
- [x] test target과 CTest를 실행해 모두 통과시킨다.

### Task 2: Win32 runtime

**Files:**
- Create: `src/main.cpp`
- Create: `resources/version.rc`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `fai::InputRouter::handle`
- Produces: `FrenchAccentInput.exe`

- [x] `main.cpp`에 single-instance mutex, tray, `WH_KEYBOARD_LL`, `WH_MOUSE_LL`, Unicode `SendInput`, message loop를 구현한다.
- [x] `FrenchAccentInput` Release x64 target을 warning-as-error로 build한다.
- [x] 앱을 시작하고 중복 실행 방지, tray 생성, 정상 종료를 확인한다.

### Task 3: installer와 release artifact

**Files:**
- Create: `installer/FrenchAccentInput.iss`
- Create: `scripts/build-release.ps1`
- Create: `.gitignore`

**Interfaces:**
- Consumes: `build/Release/FrenchAccentInput.exe`
- Produces: `dist/FrenchAccentInput-Setup-1.0.0.exe`, `dist/SHA256SUMS.txt`

- [x] Inno Setup을 설치하거나 기존 compiler를 찾는다.
- [x] per-user installer script를 compile한다.
- [x] build script로 configure, tests, app, installer, SHA-256 생성을 한 번에 재현한다.
- [x] silent install과 uninstall을 실행하고 파일·등록 상태를 확인한다.

### Task 4: 공개 문서와 학습 가이드

**Files:**
- Create: `README.md`
- Create: `docs/BUILD_GUIDE.ko.md`
- Create: `docs/TROUBLESHOOTING.md`
- Modify: `progress.md`

**Interfaces:**
- Consumes: 검증된 source, 명령, artifact 경로
- Produces: 영어 우선 bilingual README, 한국어 완전 재현 가이드, 복구 기록

- [x] README 첫 줄에 영어가 위, 한국어가 아래임을 안내한다.
- [x] 빈 Windows 환경부터 source 작성, build, test, installer, GitHub Release까지 모든 파일의 완전한 내용을 포함한 한국어 가이드를 작성한다.
- [x] 실제 실행, 정적 확인, 사용자 확인 필요 항목을 구분한다.
- [x] `progress.md`에 현재 상태, 검증 결과, 남은 수동 확인, 새 채팅 시작 문구를 기록한다.
- [x] source와 guide의 코드가 일치하는지 검사한다.
