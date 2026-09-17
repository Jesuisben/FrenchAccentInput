# 트러블슈팅 기록

## 기록 원칙

문제의 증상, 직접 원인, 확인 근거, 수정, 재검증을 순서대로 남긴다. 예상과 실제 실행 결과를 섞지 않는다.

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
