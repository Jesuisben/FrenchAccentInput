# READ FIRST

현재 단계: AUTOMATED_VALIDATION
현재 담당: CODEX
현재 전체 상태: VK 0xE8 mask + 실제 VK_BACK의 RED/GREEN은 확인했지만, 메모장 빠른 synthetic mapping에서 replacement/순서 failure가 남았다. Alt 즉시 복원을 생략해도 재현됐다. CODEX가 출력 순서 원인을 조사 중이며 사용자 작업은 없다.

## Codex가 가장 먼저 할 일

1. 이 파일을 처음부터 끝까지 읽는다.
2. `progress.md`와 `docs/TROUBLESHOOTING.md`를 읽는다.
3. 현재 담당이 CODEX이므로 `instructions/01_CODEX_CURRENT_TASK.md`를 처음부터 끝까지 읽는다.
4. 사용자에게 불필요한 수동 작업을 요구하지 않는다.
5. Codex가 자동으로 할 수 있는 작업은 직접 수행한다.
6. 사용자 물리 작업이 필요한 경우에만 `instructions/02_USER_ACTION_REQUIRED.md`를 갱신한다.
7. 사용자가 물리 검증 결과를 보내기 전에는 해당 항목을 PASS 처리하지 않는다.
8. Git/GitHub 변경은 하지 않는다.
9. 각 작업 보고에는 현재 상태와 다음 Codex 작업을 명시한다. 사용자 행동이 불가피할 때만 별도 MD 체크리스트를 안내한다.
10. 작은 build, test, 문서 갱신 하나가 끝났다고 작업을 종료하지 않는다. `01_CODEX_CURRENT_TASK.md`의 자동 작업이 남아 있으면 같은 작업 흐름에서 다음 항목을 즉시 계속 수행한다.
11. 물리 keyboard·IME·화면 검증이 아직 남았다는 사실만으로 `blocked` 처리하지 않는다. Codex가 수행 가능한 코드 조사, test target, build, lifecycle, installer, static 검증이 하나라도 남으면 먼저 끝낸다.
12. `blocked`는 동일한 외부 의존 blocker가 세 번 연속 재현되고, 다음 Codex 작업이 정확히 없을 때만 사용한다. 종료 보고에는 반드시 그 blocker와 재개 시 첫 Codex 작업을 쓴다.

## 현재 단계에서 읽어야 할 파일

- `progress.md`
- `docs/TROUBLESHOOTING.md`
- `instructions/01_CODEX_CURRENT_TASK.md`
- `src/main.cpp`
- `CMakeLists.txt`

## 사용자 안내

현재 사용자 작업 없음.
