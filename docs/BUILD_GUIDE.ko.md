# Windows 11 French Accent Input 완전 제작 가이드

이 문서는 빈 Windows 11 환경에서 시작해 TypeIt 방식의 전역 프랑스어 악상 입력기, test, standalone EXE, installer, GitHub Release까지 만드는 한 파일 완결 가이드다.

## 1. 완성 결과와 근거 수준

완성 프로그램은 기존 QWERTY·한국어 keyboard layout을 바꾸지 않는다. 왼쪽 Alt를 유지하며 `A/C/E/I/O/U/Y`를 누르면 `à â æ ç é è ê ë ï î ô œ ù û ü ÿ`를 입력한다.

근거를 다음처럼 구분한다.

- **현재 실행 검증**: 이 저장소에서 실제 실행해 성공한 build, CTest, process lifecycle, installer compile·설치·제거
- **정적 검증**: source·설정·문서의 구조를 검사한 결과
- **공식 문서 확인**: Microsoft, Inno Setup, GitHub, OpenAI 공식 문서로 확인한 사실
- **사용자 확인 필요**: software가 물리 keyboard event를 만들 수 없어 사람이 실제 keyboard로 확인해야 하는 항목
- **예상 결과**: 설명용 예시이며 실행 성공 증거가 아님

2026-09-17 현재 실행 환경에서는 Visual Studio Build Tools 18, CMake 4.2.3, MSVC 19.50, Windows SDK 10.0.26100, Inno Setup 7.1.0으로 Release build와 installer를 만들었다. CTest 1개가 통과했고, installer의 silent 설치·앱 시작·정상 종료·제거·registry 정리를 확인했다.

## 2. TypeIt에서 확인한 규칙

[TypeIt French](https://french.typeit.org/)를 직접 조작해 확인한 결과다.

| 왼쪽 Alt를 유지하며 누르는 키 | 결과 |
|---|---|
| `A`, `AA`, `AAA` | `à`, `â`, `æ` |
| `C` | 누를 때마다 `ç` 추가 |
| `E`, `EE`, `EEE`, `EEEE`, `EEEEE` | `é`, `è`, `ê`, `ë`, 다시 `é` |
| `I`, `II` | `ï`, `î` |
| `O`, `OO` | `ô`, `œ` |
| `U`, `UU`, `UUU` | `ù`, `û`, `ü` |
| `Y` | 누를 때마다 `ÿ` 추가 |
| `E`, `A` | `éà` |

사이트 source code는 사용하지 않는다. 화면에 공개된 shortcut과 관찰한 interaction만 독자적으로 구현한다.

## 3. 기술 선택

- Python은 초보자가 읽기 쉽지만 runtime·packaging dependency가 커진다.
- AutoHotkey는 짧지만 정확한 injected-event 분리와 native installer 검증이 약하다.
- C++20 + Win32는 Windows 전용이라는 범위에 맞고 추가 runtime 없이 작은 EXE를 만든다.

따라서 C++20 + Win32를 선택한다. 핵심 API는 다음과 같다.

- `WH_KEYBOARD_LL`: 전역 low-level keyboard hook
- `WH_MOUSE_LL`: mouse로 caret을 옮겼을 때 위험한 교체 상태 취소
- `SendInput`: Backspace와 Unicode 입력
- `KEYEVENTF_UNICODE`: 현재 keyboard layout과 무관한 UTF-16 입력
- `KBDLLHOOKSTRUCT`: injected event 구분
- `Shell_NotifyIconW`: tray icon
- named mutex: 중복 실행과 installer 실행 중 교체 방지

`SendInput`은 UIPI 제한을 받는다. 일반 권한 프로그램은 관리자 권한 앱에 입력할 수 없다. 이는 우회할 문제가 아니라 Windows 보안 경계다.

## 4. 준비물

### 4.1 Visual Studio Build Tools

Visual Studio Build Tools를 설치할 때 **Desktop development with C++** workload를 선택한다. 다음 구성 요소가 포함되어야 한다.

- MSVC C++ x64/x86 build tools
- Windows 11 SDK
- CMake tools for Windows

설치 후 일반 PowerShell에서 bundled CMake를 확인한다.

```powershell
Test-Path 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
```

`True`가 나와야 한다. 다른 Visual Studio version이면 실제 `cmake.exe` 경로를 build script 후보에 추가한다.

### 4.2 Inno Setup

일반 PowerShell에서 설치한다.

```powershell
winget install --id JRSoftware.InnoSetup -e --silent --accept-package-agreements --accept-source-agreements
```

2026-09-17 현재 Winget 설치 결과의 compiler는 Inno Setup 7.1.0이며 `Non-commercial use only`를 표시한다. 개인 무료 포트폴리오 범위를 넘겨 상업적으로 배포하려면 당시 license를 다시 확인한다.

### 4.3 Git과 GitHub

Git은 source version 관리, GitHub는 공개 저장소와 Release asset 배포에 사용한다. 이 프로젝트를 만드는 Agent는 commit·push·tag·Release를 실행하지 않는다. 해당 단계는 사용자가 직접 수행한다.

## 5. 최종 파일 구조

```text
FrenchAccentInput/
├── .gitignore
├── CMakeLists.txt
├── LICENSE
├── README.md
├── progress.md
├── include/fai/input_core.h
├── src/input_core.cpp
├── src/main.cpp
├── tests/input_core_tests.cpp
├── resources/version.rc
├── installer/FrenchAccentInput.iss
├── scripts/build-release.ps1
└── docs/
    ├── BUILD_GUIDE.ko.md
    ├── TROUBLESHOOTING.md
    └── superpowers/
        ├── specs/2026-09-17-french-accent-input-design.md
        └── plans/2026-09-17-french-accent-input.md
```

`build/`와 `dist/`는 명령 실행으로 생기는 산출물이다.

## 6. 빈 폴더 만들기

PowerShell에서 실행한다.

```powershell
New-Item -ItemType Directory -Force -Path 'C:\Projects\FrenchAccentInput'
Set-Location 'C:\Projects\FrenchAccentInput'
New-Item -ItemType Directory -Force -Path include\fai, src, tests, resources, installer, scripts, docs
```

이후 각 제목의 상대 경로에 code block 전체를 저장한다. 파일은 UTF-8로 저장한다.

## 7. build 설정

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(FrenchAccentInput VERSION 1.0.0 LANGUAGES CXX RC)

enable_testing()

# OS와 무관한 상태 머신을 별도 library로 두면 실제 keyboard hook 없이도 규칙을 검증할 수 있다.
add_library(fai_core STATIC src/input_core.cpp)
target_compile_features(fai_core PUBLIC cxx_std_20)
target_include_directories(fai_core PUBLIC include)

add_executable(fai_tests tests/input_core_tests.cpp)
target_compile_features(fai_tests PRIVATE cxx_std_20)
target_link_libraries(fai_tests PRIVATE fai_core)

if(MSVC)
  target_compile_options(fai_core PRIVATE /W4 /WX /permissive- /utf-8)
  target_compile_options(fai_tests PRIVATE /W4 /WX /permissive- /utf-8)
endif()

add_test(NAME input_core COMMAND fai_tests)

# WIN32 옵션은 console 창이 없는 GUI subsystem 실행 파일을 만든다.
add_executable(FrenchAccentInput WIN32 src/main.cpp resources/version.rc)
target_compile_features(FrenchAccentInput PRIVATE cxx_std_20)
target_compile_definitions(FrenchAccentInput PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
target_link_libraries(FrenchAccentInput PRIVATE fai_core user32 shell32)

if(MSVC)
  target_compile_options(FrenchAccentInput PRIVATE /W4 /WX /permissive- /utf-8)
endif()
```

`fai_core`는 운영체제에서 분리된 규칙이다. `FrenchAccentInput`과 `fai_tests`가 같은 core를 사용하므로 test가 복제 코드를 검사하지 않는다. `/W4 /WX`는 높은 경고 수준을 사용하고 모든 경고를 build 실패로 바꾼다.

## 8. 순수 입력 core

### `include/fai/input_core.h`

```cpp
#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace fai {

inline constexpr unsigned int vk_tab = 0x09;
inline constexpr unsigned int vk_f4 = 0x73;
inline constexpr unsigned int vk_left_shift = 0xA0;
inline constexpr unsigned int vk_right_shift = 0xA1;
inline constexpr unsigned int vk_left_control = 0xA2;
inline constexpr unsigned int vk_right_control = 0xA3;
inline constexpr unsigned int vk_left_alt = 0xA4;
inline constexpr unsigned int vk_right_alt = 0xA5;
inline constexpr unsigned int vk_left_windows = 0x5B;
inline constexpr unsigned int vk_right_windows = 0x5C;

struct KeyEvent {
    // Win32 event를 이 작은 값 객체로 바꿔 OS API와 판단 규칙을 분리한다.
    unsigned int virtual_key;
    bool key_down;
    bool injected;
    std::uintptr_t target;
};

struct Edit {
    // replace_previous=true면 직전에 우리가 넣은 한 글자만 Backspace로 교체한다.
    wchar_t character;
    bool replace_previous;
};

struct RouteResult {
    bool suppress = false;
    std::optional<Edit> edit;
};

class InputRouter {
public:
    // Router는 modifier 상태와 TypeIt 순환 상태를 함께 소유하는 순수 상태 머신이다.
    [[nodiscard]] RouteResult handle(const KeyEvent& event);
    void cancel_sequence() noexcept;
    void abort_consumed_key(unsigned int virtual_key) noexcept;

private:
    [[nodiscard]] bool accent_mode_active() const noexcept;
    void set_modifier(unsigned int virtual_key, bool down) noexcept;

    bool left_alt_down_ = false;
    bool right_alt_down_ = false;
    bool left_control_down_ = false;
    bool right_control_down_ = false;
    bool left_shift_down_ = false;
    bool right_shift_down_ = false;
    bool left_windows_down_ = false;
    bool right_windows_down_ = false;
    std::array<bool, 256> consumed_keys_{};

    char active_key_ = 0;
    std::size_t active_index_ = 0;
    std::uintptr_t active_target_ = 0;
};

} // namespace fai
```

`virtual key`는 Windows가 물리 key를 논리 key 번호로 표현하는 방식이다. `std::optional<Edit>`는 이번 event가 문자 편집을 만들 수도, 만들지 않을 수도 있음을 type으로 표현한다.

### `src/input_core.cpp`

```cpp
#include "fai/input_core.h"

#include <array>
#include <string_view>

namespace fai {
namespace {

struct AccentSequence {
    char key;
    std::wstring_view characters;
};

constexpr std::array sequences{
    AccentSequence{'A', L"àâæ"},
    AccentSequence{'C', L"ç"},
    AccentSequence{'E', L"éèêë"},
    AccentSequence{'I', L"ïî"},
    AccentSequence{'O', L"ôœ"},
    AccentSequence{'U', L"ùûü"},
    AccentSequence{'Y', L"ÿ"},
};

[[nodiscard]] std::wstring_view characters_for(unsigned int virtual_key) noexcept {
    for (const auto& sequence : sequences) {
        if (virtual_key == static_cast<unsigned int>(sequence.key)) {
            return sequence.characters;
        }
    }
    return {};
}

[[nodiscard]] bool is_modifier(unsigned int virtual_key) noexcept {
    switch (virtual_key) {
    case vk_left_shift:
    case vk_right_shift:
    case vk_left_control:
    case vk_right_control:
    case vk_left_alt:
    case vk_right_alt:
    case vk_left_windows:
    case vk_right_windows:
        return true;
    default:
        return false;
    }
}

} // namespace

RouteResult InputRouter::handle(const KeyEvent& event) {
    // 다른 프로그램이나 보조 기술이 만든 injected event를 다시 변환하면 재귀와 충돌이 생긴다.
    if (event.injected || event.virtual_key >= consumed_keys_.size()) {
        return {};
    }

    if (is_modifier(event.virtual_key)) {
        set_modifier(event.virtual_key, event.key_down);
        if ((event.key_down && event.virtual_key != vk_left_alt) ||
            (!event.key_down && event.virtual_key == vk_left_alt)) {
            cancel_sequence();
        }
        return {};
    }

    if (!event.key_down) {
        if (consumed_keys_[event.virtual_key]) {
            consumed_keys_[event.virtual_key] = false;
            return {.suppress = true};
        }
        return {};
    }

    const auto characters = characters_for(event.virtual_key);
    if (!accent_mode_active() || characters.empty()) {
        cancel_sequence();
        return {};
    }

    consumed_keys_[event.virtual_key] = true;

    if (characters.size() == 1) {
        cancel_sequence();
        return {.suppress = true,
                .edit = Edit{.character = characters.front(), .replace_previous = false}};
    }

    const auto key = static_cast<char>(event.virtual_key);
    const bool safe_to_replace = active_key_ == key && active_target_ != 0 &&
                                 active_target_ == event.target;
    // foreground target이 같을 때만 Backspace를 허용해 다른 앱의 문자를 지우지 않는다.
    if (safe_to_replace) {
        active_index_ = (active_index_ + 1) % characters.size();
    } else {
        active_key_ = key;
        active_index_ = 0;
        active_target_ = event.target;
    }

    return {.suppress = true,
            .edit = Edit{.character = characters[active_index_],
                         .replace_previous = safe_to_replace}};
}

void InputRouter::cancel_sequence() noexcept {
    active_key_ = 0;
    active_index_ = 0;
    active_target_ = 0;
}

void InputRouter::abort_consumed_key(unsigned int virtual_key) noexcept {
    if (virtual_key < consumed_keys_.size()) {
        consumed_keys_[virtual_key] = false;
    }
    cancel_sequence();
}

bool InputRouter::accent_mode_active() const noexcept {
    return left_alt_down_ && !right_alt_down_ && !left_control_down_ &&
           !right_control_down_ && !left_shift_down_ && !right_shift_down_ &&
           !left_windows_down_ && !right_windows_down_;
}

void InputRouter::set_modifier(unsigned int virtual_key, bool down) noexcept {
    switch (virtual_key) {
    case vk_left_alt:
        left_alt_down_ = down;
        break;
    case vk_right_alt:
        right_alt_down_ = down;
        break;
    case vk_left_control:
        left_control_down_ = down;
        break;
    case vk_right_control:
        right_control_down_ = down;
        break;
    case vk_left_shift:
        left_shift_down_ = down;
        break;
    case vk_right_shift:
        right_shift_down_ = down;
        break;
    case vk_left_windows:
        left_windows_down_ = down;
        break;
    case vk_right_windows:
        right_windows_down_ = down;
        break;
    default:
        break;
    }
}

} // namespace fai
```

핵심은 `safe_to_replace`다. 같은 문자와 같은 foreground target일 때만 Backspace를 허용한다. 앱이 바뀌거나 mouse·다른 key가 개입하면 sequence를 취소한다. 같은 앱 내부에서 keyboard만으로 caret을 옮긴 경우도 다른 key가 먼저 들어오므로 취소된다.

## 9. 자동 test

### `tests/input_core_tests.cpp`

```cpp
#include "fai/input_core.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

fai::KeyEvent down(unsigned int virtual_key, std::uintptr_t target = 1) {
    return {.virtual_key = virtual_key, .key_down = true, .injected = false, .target = target};
}

fai::KeyEvent up(unsigned int virtual_key, std::uintptr_t target = 1) {
    return {.virtual_key = virtual_key, .key_down = false, .injected = false, .target = target};
}

void typeit_sequences_cycle_and_switch() {
    // 기대값은 production mapping에서 계산하지 않고 TypeIt에서 직접 확인한 literal로 적는다.
    fai::InputRouter router;
    expect(!router.handle(down(fai::vk_left_alt)).suppress, "left Alt must reach Windows");

    const auto e1 = router.handle(down('E'));
    expect(e1.suppress && e1.edit && e1.edit->character == L'é' && !e1.edit->replace_previous,
           "Alt+E must insert é");
    expect(router.handle(up('E')).suppress, "keyup for consumed E must be suppressed");

    const auto e2 = router.handle(down('E'));
    expect(e2.edit && e2.edit->character == L'è' && e2.edit->replace_previous,
           "second E in one Alt session must replace é with è");
    (void)router.handle(up('E'));
    (void)router.handle(down('E'));
    (void)router.handle(up('E'));
    (void)router.handle(down('E'));
    (void)router.handle(up('E'));
    const auto e5 = router.handle(down('E'));
    expect(e5.edit && e5.edit->character == L'é' && e5.edit->replace_previous,
           "fifth E must wrap to é");
    (void)router.handle(up('E'));

    const auto a1 = router.handle(down('A'));
    expect(a1.edit && a1.edit->character == L'à' && !a1.edit->replace_previous,
           "switching E to A must append à");
}

void single_character_keys_append() {
    fai::InputRouter router;
    (void)router.handle(down(fai::vk_left_alt));
    const auto c1 = router.handle(down('C'));
    (void)router.handle(up('C'));
    const auto c2 = router.handle(down('C'));
    expect(c1.edit && c1.edit->character == L'ç' && !c1.edit->replace_previous,
           "first C must append ç");
    expect(c2.edit && c2.edit->character == L'ç' && !c2.edit->replace_previous,
           "repeated C must append another ç");
}

void native_shortcuts_and_injected_events_pass() {
    // 이 시험이 실패하면 악상 기능이 Windows의 기존 shortcut을 침범한 것이다.
    fai::InputRouter router;
    (void)router.handle(down(fai::vk_left_alt));
    expect(!router.handle(down(fai::vk_tab)).suppress, "Alt+Tab must pass");
    expect(!router.handle(down(fai::vk_f4)).suppress, "Alt+F4 must pass");

    fai::InputRouter ctrl_router;
    (void)ctrl_router.handle(down(fai::vk_left_alt));
    (void)ctrl_router.handle(down(fai::vk_left_control));
    expect(!ctrl_router.handle(down('E')).suppress, "Ctrl+Alt+E must pass");

    fai::InputRouter right_alt_router;
    (void)right_alt_router.handle(down(fai::vk_right_alt));
    expect(!right_alt_router.handle(down('E')).suppress, "right Alt/AltGr must pass");

    fai::InputRouter injected_router;
    const fai::KeyEvent injected{.virtual_key = 'E', .key_down = true, .injected = true, .target = 1};
    expect(!injected_router.handle(injected).suppress, "injected input must pass");
}

void unsafe_replacement_is_cancelled() {
    fai::InputRouter router;
    (void)router.handle(down(fai::vk_left_alt));
    (void)router.handle(down('E', 10));
    (void)router.handle(up('E', 10));

    const auto moved = router.handle(down('E', 20));
    expect(moved.edit && moved.edit->character == L'é' && !moved.edit->replace_previous,
           "foreground target change must start a new sequence");
    (void)router.handle(up('E', 20));

    router.cancel_sequence();
    const auto cancelled = router.handle(down('E', 20));
    expect(cancelled.edit && cancelled.edit->character == L'é' && !cancelled.edit->replace_previous,
           "mouse or focus cancellation must prevent stale Backspace");
}

void alt_release_starts_new_session() {
    fai::InputRouter router;
    (void)router.handle(down(fai::vk_left_alt));
    (void)router.handle(down('U'));
    (void)router.handle(up('U'));
    (void)router.handle(up(fai::vk_left_alt));
    (void)router.handle(down(fai::vk_left_alt));
    const auto fresh = router.handle(down('U'));
    expect(fresh.edit && fresh.edit->character == L'ù' && !fresh.edit->replace_previous,
           "releasing Alt must reset sequence");
}

void failed_output_restores_fail_open_routing() {
    fai::InputRouter router;
    (void)router.handle(down(fai::vk_left_alt));
    const auto accent = router.handle(down('E'));
    expect(accent.suppress, "accent keydown must initially be consumed");

    router.abort_consumed_key('E');
    expect(!router.handle(up('E')).suppress,
           "keyup must pass after output injection failed and keydown was released");
}

} // namespace

int main() {
    typeit_sequences_cycle_and_switch();
    single_character_keys_append();
    native_shortcuts_and_injected_events_pass();
    unsafe_replacement_is_cancelled();
    alt_release_starts_new_session();
    failed_output_restores_fail_open_routing();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All input core tests passed\n";
    return EXIT_SUCCESS;
}
```

test는 TypeIt 순서, 기존 shortcut 비간섭, injected event, target 변경, Alt session 종료, `SendInput` 실패 후 fail-open을 검사한다. 별도 test framework를 추가하지 않고 exit code로 CTest와 연결한다.

## 10. Win32 runtime

### `src/main.cpp`

```cpp
#include "fai/input_core.h"

#include <windows.h>
#include <shellapi.h>

#include <cstdint>
#include <cwchar>
#include <vector>

namespace {

constexpr wchar_t app_name[] = L"French Accent Input";
constexpr wchar_t window_class_name[] = L"FrenchAccentInput.HiddenWindow";
constexpr wchar_t mutex_name[] = L"Local\\FrenchAccentInput-4A67FCE1-5DC0-4ACB-9843-9672E4CBE071";
constexpr ULONG_PTR injection_marker = 0x46414931;
constexpr UINT tray_icon_id = 1;
constexpr UINT tray_callback_message = WM_APP + 1;
constexpr UINT input_warning_message = WM_APP + 2;
constexpr UINT exit_command_id = 1001;

HHOOK keyboard_hook = nullptr;
HHOOK mouse_hook = nullptr;
HWND hidden_window = nullptr;
UINT taskbar_created_message = 0;
fai::InputRouter router;
bool warning_shown = false;

INPUT virtual_key_input(WORD virtual_key, DWORD flags) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = virtual_key;
    input.ki.dwFlags = flags;
    input.ki.dwExtraInfo = injection_marker;
    return input;
}

INPUT unicode_input(wchar_t character, DWORD flags) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = static_cast<WORD>(character);
    input.ki.dwFlags = KEYEVENTF_UNICODE | flags;
    input.ki.dwExtraInfo = injection_marker;
    return input;
}

enum class SendStatus { success, blocked, partial };

SendStatus send_edit(const fai::Edit& edit) {
    std::vector<INPUT> inputs;
    inputs.reserve(edit.replace_previous ? 7U : 5U);

    // Alt가 눌린 상태에서는 Backspace가 앱별 단축키가 될 수 있으므로 잠시 해제한다.
    inputs.push_back(virtual_key_input(VK_LMENU, KEYEVENTF_KEYUP));
    if (edit.replace_previous) {
        inputs.push_back(virtual_key_input(VK_BACK, 0));
        inputs.push_back(virtual_key_input(VK_BACK, KEYEVENTF_KEYUP));
    }

    // KEYEVENTF_UNICODE는 현재 keyboard layout과 무관하게 UTF-16 문자를 전달한다.
    inputs.push_back(unicode_input(edit.character, 0));
    inputs.push_back(unicode_input(edit.character, KEYEVENTF_KEYUP));
    inputs.push_back(virtual_key_input(VK_LMENU, 0));

    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent == inputs.size()) {
        return SendStatus::success;
    }
    if (sent == 0) {
        return SendStatus::blocked;
    }

    // 부분 전송은 Alt가 논리적으로 올라간 채 남을 수 있다. 복구 입력은 best effort다.
    auto restore_alt = virtual_key_input(VK_LMENU, 0);
    (void)SendInput(1, &restore_alt, sizeof(INPUT));
    return SendStatus::partial;
}

void show_input_warning() {
    if (warning_shown || hidden_window == nullptr) {
        return;
    }
    warning_shown = true;

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hidden_window;
    data.uID = tray_icon_id;
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIIF_WARNING;
    wcscpy_s(data.szInfoTitle, L"French Accent Input");
    wcscpy_s(data.szInfo,
             L"문자 입력이 Windows에서 차단되었습니다. 관리자 권한 앱에는 입력할 수 없습니다.");
    (void)Shell_NotifyIconW(NIM_MODIFY, &data);
}

bool add_tray_icon() {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hidden_window;
    data.uID = tray_icon_id;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = tray_callback_message;
    data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(data.szTip, L"French Accent Input — Left Alt + A/C/E/I/O/U/Y");

    if (!Shell_NotifyIconW(NIM_ADD, &data)) {
        return false;
    }
    data.uVersion = NOTIFYICON_VERSION_4;
    (void)Shell_NotifyIconW(NIM_SETVERSION, &data);
    return true;
}

void remove_tray_icon() {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hidden_window;
    data.uID = tray_icon_id;
    (void)Shell_NotifyIconW(NIM_DELETE, &data);
}

void show_tray_menu() {
    POINT point{};
    if (!GetCursorPos(&point)) {
        return;
    }

    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    (void)AppendMenuW(menu, MF_STRING, exit_command_id, L"Exit / 종료");
    (void)SetForegroundWindow(hidden_window);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        point.x, point.y, 0, hidden_window, nullptr);
    DestroyMenu(menu);
    if (command == exit_command_id) {
        DestroyWindow(hidden_window);
    }
}

LRESULT CALLBACK keyboard_proc(int code, WPARAM message, LPARAM event_data) {
    if (code < 0) {
        return CallNextHookEx(keyboard_hook, code, message, event_data);
    }

    const auto* data = reinterpret_cast<const KBDLLHOOKSTRUCT*>(event_data);
    const bool key_down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool key_up = message == WM_KEYUP || message == WM_SYSKEYUP;
    if (!key_down && !key_up) {
        return CallNextHookEx(keyboard_hook, code, message, event_data);
    }

    const bool injected = (data->flags & LLKHF_INJECTED) != 0 ||
                          data->dwExtraInfo == injection_marker;
    const fai::KeyEvent event{
        .virtual_key = data->vkCode,
        .key_down = key_down,
        .injected = injected,
        .target = reinterpret_cast<std::uintptr_t>(GetForegroundWindow()),
    };
    const auto route = router.handle(event);

    if (route.edit) {
        const auto status = send_edit(*route.edit);
        if (status == SendStatus::blocked) {
            router.abort_consumed_key(data->vkCode);
            (void)PostMessageW(hidden_window, input_warning_message, 0, 0);
            return CallNextHookEx(keyboard_hook, code, message, event_data);
        }
        if (status == SendStatus::partial) {
            router.cancel_sequence();
            (void)PostMessageW(hidden_window, input_warning_message, 0, 0);
        }
    }

    if (route.suppress) {
        return 1;
    }
    return CallNextHookEx(keyboard_hook, code, message, event_data);
}

LRESULT CALLBACK mouse_proc(int code, WPARAM message, LPARAM event_data) {
    if (code >= 0) {
        const auto* data = reinterpret_cast<const MSLLHOOKSTRUCT*>(event_data);
        const bool injected = (data->flags & LLMHF_INJECTED) != 0;
        const bool changes_caret = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                                   message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN ||
                                   message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
        if (!injected && changes_caret) {
            router.cancel_sequence();
        }
    }
    return CallNextHookEx(mouse_hook, code, message, event_data);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == taskbar_created_message) {
        (void)add_tray_icon();
        return 0;
    }

    switch (message) {
    case tray_callback_message:
        if (LOWORD(lparam) == WM_RBUTTONUP || LOWORD(lparam) == WM_CONTEXTMENU) {
            show_tray_menu();
        }
        return 0;
    case input_warning_message:
        show_input_warning();
        return 0;
    case WM_DESTROY:
        remove_tray_icon();
        if (mouse_hook != nullptr) {
            UnhookWindowsHookEx(mouse_hook);
            mouse_hook = nullptr;
        }
        if (keyboard_hook != nullptr) {
            UnhookWindowsHookEx(keyboard_hook);
            keyboard_hook = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wparam, lparam);
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command_line, int) {
    if (command_line != nullptr && wcsstr(command_line, L"--quit-existing") != nullptr) {
        const HWND existing = FindWindowW(window_class_name, nullptr);
        if (existing == nullptr) {
            return 2;
        }
        return PostMessageW(existing, WM_CLOSE, 0, 0) ? 0 : 1;
    }

    HANDLE mutex = CreateMutexW(nullptr, TRUE, mutex_name);
    if (mutex == nullptr) {
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return 0;
    }

    taskbar_created_message = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = window_class_name;

    if (RegisterClassExW(&window_class) == 0) {
        CloseHandle(mutex);
        return 1;
    }

    hidden_window = CreateWindowExW(0, window_class_name, app_name, WS_OVERLAPPED,
                                    0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (hidden_window == nullptr || !add_tray_icon()) {
        if (hidden_window != nullptr) {
            DestroyWindow(hidden_window);
        }
        CloseHandle(mutex);
        return 1;
    }

    keyboard_hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_proc, instance, 0);
    mouse_hook = SetWindowsHookExW(WH_MOUSE_LL, mouse_proc, instance, 0);
    if (keyboard_hook == nullptr || mouse_hook == nullptr) {
        MessageBoxW(nullptr, L"Windows 입력 hook을 시작하지 못했습니다.", app_name,
                    MB_OK | MB_ICONERROR);
        DestroyWindow(hidden_window);
        CloseHandle(mutex);
        return 1;
    }

    MSG message{};
    BOOL message_result = 0;
    while ((message_result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    CloseHandle(mutex);
    if (message_result == -1) {
        return 1;
    }
    return static_cast<int>(message.wParam);
}
```

`hook`은 Windows가 입력 event를 target queue에 넣기 직전에 호출하는 callback이다. 처리하지 않은 event는 `CallNextHookEx`로 넘긴다. 처리한 악상 keydown·keyup만 `1`을 반환해 target 앱의 원래 shortcut보다 우선한다.

`dwExtraInfo` marker와 `LLKHF_INJECTED` 검사는 프로그램이 보낸 Unicode event를 다시 처리하는 재귀를 막는다. `SendInput`이 0개를 보내면 원래 event를 통과시키는 fail-open으로 동작한다.

## 11. Windows version resource

### `resources/version.rc`

```rc
#include <windows.h>

VS_VERSION_INFO VERSIONINFO
 FILEVERSION 1,0,0,0
 PRODUCTVERSION 1,0,0,0
 FILEFLAGSMASK 0x3fL
 FILEFLAGS 0x0L
 FILEOS VOS_NT_WINDOWS32
 FILETYPE VFT_APP
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0"
        BEGIN
            VALUE "CompanyName", "French Accent Input contributors\0"
            VALUE "FileDescription", "TypeIt-style French accent input for Windows\0"
            VALUE "FileVersion", "1.0.0\0"
            VALUE "InternalName", "FrenchAccentInput\0"
            VALUE "OriginalFilename", "FrenchAccentInput.exe\0"
            VALUE "ProductName", "French Accent Input\0"
            VALUE "ProductVersion", "1.0.0\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
```

resource file은 EXE 속성의 제품명과 version을 만든다. source나 installer version을 올릴 때 이 숫자도 같이 바꾼다.

## 12. installer

### `installer/FrenchAccentInput.iss`

```ini
#define AppName "French Accent Input"
#define AppVersion "1.0.0"
#define AppExeName "FrenchAccentInput.exe"
#define AppMutex "Local\FrenchAccentInput-4A67FCE1-5DC0-4ACB-9843-9672E4CBE071"

[Setup]
; 고정 AppId는 다음 버전 installer가 기존 설치를 upgrade하도록 같은 제품 계보를 만든다.
AppId={{4A67FCE1-5DC0-4ACB-9843-9672E4CBE071}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=French Accent Input contributors
DefaultDirName={localappdata}\Programs\FrenchAccentInput
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
; 사용자별 경로만 쓰므로 UAC 관리자 승격이 필요 없다.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir=..\dist
OutputBaseFilename=FrenchAccentInput-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupLogging=yes
Uninstallable=yes
UninstallDisplayIcon={app}\{#AppExeName}
LicenseFile=..\LICENSE
AppMutex={#AppMutex}
; 실행 중인 hook을 둔 채 파일을 교체하거나 제거하지 못하게 같은 mutex를 검사한다.
CloseApplications=yes
RestartApplications=no
VersionInfoVersion=1.0.0.0
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Files]
Source: "..\build\Release\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent
```

`PrivilegesRequired=lowest`는 per-user 설치를 뜻한다. `AppId`는 upgrade 계보와 uninstall registry key를 결정하므로 version마다 바꾸지 않는다. `AppMutex`는 실행 중인 앱을 installer와 uninstaller가 감지하게 한다.

## 13. 한 번에 build하는 script

### `scripts/build-release.ps1`

```powershell
param(
    [switch]$SkipInstaller
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $ProjectRoot

function Find-Tool {
    param([string]$Name, [string[]]$Candidates)

    $Command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($Command) { return $Command.Source }

    foreach ($Candidate in $Candidates) {
        if (Test-Path -LiteralPath $Candidate) { return $Candidate }
    }
    throw "$Name 도구를 찾지 못했습니다. docs/BUILD_GUIDE.ko.md의 설치 절차를 확인하세요."
}

# PATH에 없는 Visual Studio bundled CMake도 같은 명령으로 찾는다.
$CMake = Find-Tool 'cmake' @(
    'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
)
$CTest = Join-Path (Split-Path -Parent $CMake) 'ctest.exe'

& $CMake -S . -B build -G 'Visual Studio 18 2026' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure 실패' }

& $CMake --build build --config Release --target FrenchAccentInput fai_tests
if ($LASTEXITCODE -ne 0) { throw 'Release build 실패' }

& $CTest --test-dir build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'CTest 실패' }

New-Item -ItemType Directory -Path dist -Force | Out-Null
Copy-Item -LiteralPath 'build\Release\FrenchAccentInput.exe' -Destination 'dist\FrenchAccentInput.exe' -Force

if (-not $SkipInstaller) {
    $ISCC = Find-Tool 'ISCC' @(
        'C:\Program Files\Inno Setup 7\ISCC.exe',
        'C:\Program Files (x86)\Inno Setup 7\ISCC.exe',
        'C:\Program Files\Inno Setup 6\ISCC.exe',
        'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
    )
    & $ISCC 'installer\FrenchAccentInput.iss'
    if ($LASTEXITCODE -ne 0) { throw 'Inno Setup compile 실패' }
}

$Artifacts = Get-ChildItem -LiteralPath dist -File | Where-Object Extension -eq '.exe' | Sort-Object Name
$Lines = foreach ($Artifact in $Artifacts) {
    # Release 사용자는 다운로드 파일이 제작자가 만든 byte와 같은지 SHA-256으로 확인한다.
    $Hash = (Get-FileHash -LiteralPath $Artifact.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$Hash  $($Artifact.Name)"
}
$Lines | Set-Content -LiteralPath 'dist\SHA256SUMS.txt' -Encoding ascii

$Artifacts | Select-Object Name, Length, LastWriteTime
Get-Content -LiteralPath 'dist\SHA256SUMS.txt'
```

이 script는 configure, Release build, CTest, installer compile, checksum을 한 명령으로 묶는다. error가 하나라도 나면 `$ErrorActionPreference='Stop'`과 exit code 검사로 즉시 중단한다.

## 14. 저장소 제외 파일과 license

### `.gitignore`

```gitignore
/build/
/dist/
/.vs/
/out/
*.user
*.suo
*.log
```

build 결과와 개인 IDE 상태를 공개 source에 올리지 않는다.

### `LICENSE`

```text
MIT License

Copyright (c) 2026 French Accent Input contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

MIT License는 source 사용·수정·재배포를 허용하고 저작권·허가문 유지와 무보증 조건을 요구한다. 개인 이름 대신 contributors 표기를 사용해 개인 정보를 넣지 않았다.

## 15. build, test, 실행, 종료

프로젝트 루트의 일반 PowerShell에서 실행한다.

```powershell
.\scripts\build-release.ps1
```

성공하면 다음 파일이 생긴다.

```text
dist\FrenchAccentInput.exe
dist\FrenchAccentInput-Setup-1.0.0.exe
dist\SHA256SUMS.txt
```

installer 없이 개발용 EXE를 시작한다.

```powershell
Start-Process '.\dist\FrenchAccentInput.exe'
```

tray menu를 쓰지 않고 script에서 정상 종료하려면 다음 command를 사용한다.

```powershell
& '.\dist\FrenchAccentInput.exe' --quit-existing
```

종료 code `0`은 실행 중인 instance에 종료 message를 보냈다는 뜻이다. 실행 중인 instance가 없으면 code `2`다.

CTest만 다시 실행한다.

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir build -C Release --output-on-failure
```

## 16. 물리 keyboard 수동 검증

`SendInput` 같은 software 주입은 low-level hook에서 injected로 표시된다. 따라서 자동화 도구가 만든 key는 물리 keyboard 동작의 증거가 될 수 없다. Release 전에 실제 keyboard로 다음을 한 번 확인한다.

1. `dist\FrenchAccentInput.exe`를 실행한다.
2. 메모장에 빈 문서를 연다.
3. 왼쪽 Alt를 누른 채 `E`를 1~5회 누른다. 각 시점의 target-visible text가 `é`, `è`, `ê`, `ë`, `é`인지 확인한다.
4. Alt를 놓고 다시 Alt를 누른 채 `E`, `A`를 누른다. `éà`인지 확인한다.
5. Alt를 유지하며 `C`를 세 번 누른다. `ççç`인지 확인한다.
6. `A`, `I`, `O`, `U`, `Y`의 표 전체를 확인한다.
7. `Alt+Tab`, `Alt+F4`, `Ctrl+A/C/V`, 오른쪽 Alt/AltGr가 기존대로인지 확인한다.
8. 한국어 IME에서 한글 조합을 끝낸 뒤 악상 입력이 되는지 확인한다. 조합 중 동작은 별도 기록한다.
9. browser, Word 또는 VS Code에서도 target-visible text를 확인한다.
10. tray icon을 오른쪽 click하고 `Exit / 종료`로 process가 끝나는지 확인한다.

한 글자라도 이전 위치에서 잘못 지워지는 stale replacement가 나오면 Release를 중단한다. 앱·입력 언어·정확한 key 순서·보인 text를 `docs/TROUBLESHOOTING.md`에 기록하고 원인을 고친 뒤 전체 matrix를 다시 실행한다.

## 17. installer 설치·제거 검증

먼저 실행 중인 개발용 instance를 종료한다.

```powershell
& '.\dist\FrenchAccentInput.exe' --quit-existing
```

아무 instance도 없어서 exit code `2`가 나오는 것은 오류가 아니다. 이어서 현재 사용자에게 조용히 설치한다.

```powershell
$Installer = Resolve-Path '.\dist\FrenchAccentInput-Setup-1.0.0.exe'
$Install = Start-Process $Installer -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -Wait -PassThru
if ($Install.ExitCode -ne 0) { throw "installer 실패: $($Install.ExitCode)" }
```

설치 파일과 Windows의 제거 등록을 확인한다.

```powershell
$InstalledExe = Join-Path $env:LOCALAPPDATA 'Programs\FrenchAccentInput\FrenchAccentInput.exe'
if (-not (Test-Path -LiteralPath $InstalledExe)) { throw '설치 EXE 없음' }

$UninstallKey = Get-ChildItem 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall' |
    Get-ItemProperty |
    Where-Object DisplayName -like 'French Accent Input*' |
    Select-Object -First 1
if (-not $UninstallKey) { throw '제거 registry 항목 없음' }
$UninstallKey | Select-Object DisplayName, DisplayVersion, UninstallString
```

설치본의 process lifecycle을 확인한다.

```powershell
$Process = Start-Process $InstalledExe -PassThru
Start-Sleep -Milliseconds 500
& $InstalledExe --quit-existing
$Process.WaitForExit(5000)
if (-not $Process.HasExited) { throw '설치본이 정상 종료되지 않음' }
```

마지막으로 제거하고 잔여 상태를 확인한다. 이 명령은 이 가이드대로 만든 시험 설치만 제거한다.

```powershell
$Uninstaller = Join-Path $env:LOCALAPPDATA 'Programs\FrenchAccentInput\unins000.exe'
$Remove = Start-Process $Uninstaller -ArgumentList '/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART' -Wait -PassThru
if ($Remove.ExitCode -ne 0) { throw "uninstaller 실패: $($Remove.ExitCode)" }

if (Test-Path (Join-Path $env:LOCALAPPDATA 'Programs\FrenchAccentInput')) {
    throw '설치 폴더가 남음'
}
$Remaining = Get-ChildItem 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall' |
    Get-ItemProperty |
    Where-Object DisplayName -like 'French Accent Input*'
if ($Remaining) { throw '제거 registry 항목이 남음' }
```

`AppId`는 version이 올라가도 바꾸지 않는다. 그래야 다음 installer가 같은 제품을 upgrade하고 기존 제거 항목을 이어받는다.

## 18. GitHub 공개와 Release 배포

Agent는 이 절차를 실행하지 않는다. 다음 단계는 저장소 소유자가 GitHub에서 직접 수행한다.

1. 공개 저장소를 만들기 전에 `git status`로 `build/`, `dist/`, 개인 IDE 파일이 추적되지 않는지 확인한다.
2. source와 문서를 검토하고 직접 commit·push한다.
3. GitHub 저장소의 **Releases → Draft a new release**를 연다.
4. source의 version과 같은 tag, 예를 들어 `v1.0.0`을 만든다.
5. 제목을 `French Accent Input 1.0.0`으로 적고 지원 OS, shortcut, 주요 제한, 물리 keyboard 검증 결과를 release note에 적는다.
6. `dist\FrenchAccentInput-Setup-1.0.0.exe`, 선택적으로 `dist\FrenchAccentInput.exe`, `dist\SHA256SUMS.txt`를 첨부한다.
7. 첨부 파일 이름과 `SHA256SUMS.txt`가 최종 build 결과와 같은지 확인한 뒤 publish한다.

사용자는 installer와 checksum을 같은 Release에서 받아 다음처럼 확인할 수 있다.

```powershell
$Expected = (Get-Content '.\SHA256SUMS.txt' | Where-Object { $_ -match 'FrenchAccentInput-Setup-1\.0\.0\.exe$' }).Split()[0]
$Actual = (Get-FileHash '.\FrenchAccentInput-Setup-1.0.0.exe' -Algorithm SHA256).Hash.ToLowerInvariant()
if ($Actual -ne $Expected) { throw 'checksum 불일치: 실행하지 마세요.' }
```

tag는 특정 source 시점을 가리키고 Release asset은 그 시점에 배포하는 binary다. source를 고친 뒤 예전 이름의 asset만 교체하지 말고 version을 올려 새 build와 Release를 만든다.

## 19. 보안·개인정보·현실적인 제한

- code signing certificate가 없으므로 SmartScreen 경고가 나올 수 있다. checksum은 전송 중 변조를 찾지만 게시자의 신원을 보증하지는 않는다.
- 프로그램은 network, telemetry, 계정, 자동 update를 사용하지 않는다.
- 입력 문자, 전체 key sequence, window title, timestamp를 file이나 registry에 저장하지 않는다.
- low-level hook은 전역 keyboard event를 관찰하지만 처리 판단은 왼쪽 Alt와 고정된 7개 문자에만 사용한다.
- Windows UIPI 때문에 일반 권한 프로그램은 관리자 권한 app에 `SendInput`을 넣지 못한다.
- secure desktop, anti-cheat game, remote session, 모든 custom editor의 동작은 보장하지 않는다.
- 현재 version은 소문자만 제공한다. 대문자·설정 UI·자동 시작·자동 update는 실제 요구가 확인되기 전까지 넣지 않는다.

## 20. Codex model, 사용량, 중단 후 이어가기

이 프로젝트 하나를 같은 설정으로 진행하려면 **GPT-5.6 Terra + Medium reasoning**을 권장한다. Terra는 일상적인 agentic coding에 맞는 균형형이고, Medium은 설계·C++·Windows API·installer 검증에 필요한 판단력을 유지하면서 높은 reasoning보다 사용량을 아낀다. 실제 사용 가능 model과 limit은 계정·시점에 따라 달라지므로 Codex의 usage 화면에서 확인한다.

사용량이 거의 끝났을 때는 새 기능을 억지로 시작하지 않는다. 현재 단계의 test 결과와 다음 한 작업을 `progress.md`에 먼저 기록하고 멈춘다. limit이 reset된 뒤 같은 프로젝트를 열고 다음 문장으로 시작한다.

```text
C:\myDdrive\FrenchAccentInput\progress.md를 읽고, Git/GitHub를 변경하지 말고 다음 미완료 작업부터 자동으로 계속해. 중간 질문 없이 검증 가능한 범위까지 진행해.
```

이 문장은 이전 대화 전체를 복사하라는 뜻이 아니다. `progress.md`가 완료·미완료·검증 근거·다음 작업의 단일 인계 문서다. Agent는 실제 파일을 다시 확인한 뒤 이어간다.

## 21. 문제 해결 표

자세한 조사 기록은 `docs/TROUBLESHOOTING.md`에 누적한다.

| 증상 | 먼저 볼 것 | 해결 방향 |
|---|---|---|
| `cmake`를 찾지 못함 | Visual Studio Build Tools와 CMake component | bundled CMake 경로 사용 |
| compile warning이 오류가 됨 | `/W4 /WX`가 표시한 첫 경고 | warning을 숨기지 말고 원인 수정 |
| `CVT1100`, `LNK1123` | manifest resource 중복 | default manifest와 custom manifest 중 하나만 사용 |
| 악상 입력이 안 됨 | 왼쪽 Alt인지, 대상 app이 관리자 권한인지 | 일반 권한 app에서 재현; UIPI 경계 확인 |
| 직전이 아닌 문자가 지워짐 | 정확한 app·caret 이동·key 순서 | Release 중단, stale replacement를 회귀 test로 고정 |
| installer가 실행 중 앱을 막음 | tray에 instance가 남았는지 | `--quit-existing` 또는 tray Exit 후 재시도 |
| registry 검증이 항목을 못 찾음 | 실제 `DisplayName`에 version 포함 여부 | `French Accent Input*`으로 확인 |

문제를 고칠 때는 증상만 우회하지 않는다. 재현 → 직접 원인 → 가장 좁은 책임 지점 수정 → 회귀 test 또는 실제 재검증 순서를 따른다.

## 22. 용어 풀이

- **Hook**: Windows event 흐름에서 keyboard·mouse event를 관찰하거나 처리하는 callback 연결점이다.
- **Callback**: Windows가 event 발생 시 프로그램의 정해진 function을 호출하는 방식이다.
- **Injected event**: 물리 keyboard가 아니라 software가 만든 입력 event다.
- **Virtual key**: Windows가 key를 표현하는 논리 번호다.
- **Unicode / UTF-16**: `é` 같은 문자를 code point와 16-bit code unit으로 표현하는 표준 방식이다.
- **State machine**: 이전 입력 상태에 따라 다음 결과가 달라지는 규칙 묶음이다.
- **Foreground window**: 현재 사용자가 입력 중인 앞쪽 window다.
- **Mutex**: 같은 프로그램이 동시에 두 번 실행되는 것을 막는 OS 이름표다.
- **System tray**: taskbar 알림 영역이다.
- **UIPI**: 낮은 권한 process가 높은 권한 process에 입력·message를 보내지 못하게 하는 Windows 보안 경계다.
- **Installer**: EXE를 정해진 위치에 복사하고 시작 메뉴·제거 정보를 등록하는 설치 프로그램이다.
- **Registry**: Windows와 app의 구조화된 설정 database다.
- **Checksum / SHA-256**: file byte가 같은지 확인하는 고정 길이 hash다.
- **Release**: 특정 version의 설명과 배포 binary를 묶어 공개하는 GitHub 기능이다.

## 23. 최종 점검표

- [ ] `scripts\build-release.ps1`가 configure, Release build, CTest, installer compile, SHA-256 생성까지 성공한다.
- [ ] 중복 실행 방지와 `--quit-existing`가 동작한다.
- [ ] silent 설치·실행·제거 뒤 file과 registry가 정리된다.
- [ ] 물리 keyboard로 전체 mapping과 순환을 확인한다.
- [ ] 메모장·browser·Word 또는 VS Code에서 target-visible text를 확인한다.
- [ ] `Alt+Tab`, `Alt+F4`, Ctrl shortcut, 오른쪽 Alt/AltGr가 유지된다.
- [ ] 한국어 IME 상태에서 확인한다.
- [ ] 개인 정보·credential·local absolute user path가 공개 source에 없다.
- [ ] README의 version, file name, 제한, 명령이 최종 결과와 일치한다.
- [ ] 사용자가 직접 commit·push·tag·GitHub Release를 수행한다.

## 24. 다음 Agent용 구현 인계

이 가이드를 실제 프로젝트로 구현하는 Agent는 다음 경계를 지킨다.

1. 먼저 대상 project의 `progress.md`, 실제 source, `git status`를 읽는다.
2. Git commit·push·tag·Release는 수행하지 않는다.
3. mapping이나 routing 변경은 `tests/input_core_tests.cpp`에 실패하는 사례를 먼저 추가한다.
4. synthetic key injection을 물리 keyboard 성공 근거로 쓰지 않는다.
5. build·CTest·installer lifecycle을 실행하고 실제 exit code와 산출물을 기록한다.
6. 자동화할 수 없는 물리 keyboard·IME 검증은 미완료라고 명확히 남긴다.
7. 단계가 끝날 때마다 `progress.md`와 실제 문제를 `docs/TROUBLESHOOTING.md`에 갱신한다.

## 25. 공식 참고 자료

- TypeIt French: <https://french.typeit.org/>
- Microsoft `LowLevelKeyboardProc`: <https://learn.microsoft.com/windows/win32/winmsg/lowlevelkeyboardproc>
- Microsoft `SendInput`: <https://learn.microsoft.com/windows/win32/api/winuser/nf-winuser-sendinput>
- Microsoft `KBDLLHOOKSTRUCT`: <https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-kbdllhookstruct>
- Microsoft `KEYBDINPUT`: <https://learn.microsoft.com/windows/win32/api/winuser/ns-winuser-keybdinput>
- Inno Setup `AppId`: <https://jrsoftware.org/ishelp/topic_setup_appid.htm>
- Inno Setup `PrivilegesRequired`: <https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm>
- Inno Setup `AppMutex`: <https://jrsoftware.org/ishelp/topic_setup_appmutex.htm>
- GitHub Release 관리: <https://docs.github.com/repositories/releasing-projects-on-github/managing-releases-in-a-repository>
- OpenAI Codex pricing: <https://learn.chatgpt.com/ko-KR/docs/pricing>
- OpenAI model 안내: <https://developers.openai.com/api/docs/guides/latest-model>
