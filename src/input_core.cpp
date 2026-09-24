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
        if (!event.key_down && event.virtual_key == vk_left_alt && left_alt_used_for_accent_) {
            left_alt_used_for_accent_ = false;
            cancel_sequence();
            return {.suppress = true, .mask_left_alt_release = true};
        }
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
    if (!event.accent_allowed || !accent_mode_active() || characters.empty()) {
        cancel_sequence();
        return {};
    }

    consumed_keys_[event.virtual_key] = true;
    left_alt_used_for_accent_ = true;

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
    left_alt_used_for_accent_ = false;
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
