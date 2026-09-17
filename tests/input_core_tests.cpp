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
