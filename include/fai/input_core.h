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
    bool mask_left_alt_release = false;
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
    bool left_alt_used_for_accent_ = false;
    std::array<bool, 256> consumed_keys_{};

    char active_key_ = 0;
    std::size_t active_index_ = 0;
    std::uintptr_t active_target_ = 0;
};

} // namespace fai
