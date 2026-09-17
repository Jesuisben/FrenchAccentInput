#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {
std::vector<INPUT> captured_inputs;
UINT accepted_count = (std::numeric_limits<UINT>::max)();
UINT WINAPI capture_send_input(UINT count, LPINPUT inputs, int size) {
    if (size != sizeof(INPUT)) {
        return 0;
    }
    captured_inputs.assign(inputs, inputs + count);
    return accepted_count < count ? accepted_count : count;
}
}

// Production output construction is compiled unchanged; no input reaches Windows.
#define SendInput capture_send_input
#include "../src/main.cpp"
#undef SendInput

namespace {
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expect_key(std::size_t index, WORD key, DWORD flags) {
    const auto& input = captured_inputs.at(index);
    expect(input.type == INPUT_KEYBOARD && input.ki.wVk == key &&
               input.ki.dwFlags == flags && input.ki.dwExtraInfo == injection_marker,
           "modifier event must have the exact key, flags and recursion marker");
}

void expect_character(std::size_t index, wchar_t character, DWORD flags) {
    const auto& input = captured_inputs.at(index);
    expect(input.type == INPUT_KEYBOARD && input.ki.wVk == 0 &&
               input.ki.wScan == static_cast<WORD>(character) &&
               input.ki.dwFlags == (KEYEVENTF_UNICODE | flags) &&
               input.ki.dwExtraInfo == injection_marker,
           "Unicode event must have the exact character, flags and recursion marker");
}

void output_contract() {
    for (const bool replace : {false, true}) {
        accepted_count = (std::numeric_limits<UINT>::max)();
        expect(send_edit({L'è', replace}) == SendStatus::success, "full output must succeed");
        const std::size_t count = replace ? 8U : 6U;
        expect(captured_inputs.size() == count, "output event count");
        expect_key(0, 0xE8, 0);
        expect_key(1, VK_LMENU, KEYEVENTF_KEYUP);
        expect_key(2, 0xE8, KEYEVENTF_KEYUP);
        if (replace) {
            expect_key(3, VK_BACK, 0);
            expect_key(4, VK_BACK, KEYEVENTF_KEYUP);
        }
        expect_character(count - 3, L'è', 0);
        expect_character(count - 2, L'è', KEYEVENTF_KEYUP);
        expect_key(count - 1, VK_LMENU, 0);
        for (UINT accepted = 0; accepted < count; ++accepted) {
            accepted_count = accepted;
            expect(send_edit({L'è', replace}) ==
                       (accepted == 0 ? SendStatus::blocked : SendStatus::partial),
                   "every short SendInput result must be reported as failure");
        }
    }
    accepted_count = (std::numeric_limits<UINT>::max)();
    expect(send_masked_alt_release() == SendStatus::success, "masked release succeeds");
    expect(captured_inputs.size() == 3, "masked release event count");
    expect_key(0, 0xE8, 0);
    expect_key(1, VK_LMENU, KEYEVENTF_KEYUP);
    expect_key(2, 0xE8, KEYEVENTF_KEYUP);
    for (UINT accepted = 0; accepted < 3; ++accepted) {
        accepted_count = accepted;
        expect(send_masked_alt_release() ==
                   (accepted == 0 ? SendStatus::blocked : SendStatus::partial),
               "short masked release must be reported as failure");
    }
}
}

int main() {
    output_contract();
    if (failures != 0) {
        return EXIT_FAILURE;
    }
    std::cout << "Win32 output contract passed (SendInput mocked; no desktop input)\n";
    return EXIT_SUCCESS;
}
