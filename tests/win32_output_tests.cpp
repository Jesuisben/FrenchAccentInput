#include <windows.h>
#include <shellapi.h>
#include <richedit.h>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
std::vector<INPUT> captured_inputs;
UINT accepted_count = (std::numeric_limits<UINT>::max)();
HWND test_focus = nullptr;
HWND WINAPI test_foreground() { return test_focus; }
BOOL WINAPI test_gui_thread(DWORD, PGUITHREADINFO info) {
    if (test_focus == nullptr) { return FALSE; }
    info->hwndFocus = test_focus;
    return TRUE;
}
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
#define GetForegroundWindow test_foreground
#define GetGUIThreadInfo test_gui_thread
#include "../src/main.cpp"
#undef GetGUIThreadInfo
#undef GetForegroundWindow
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

void native_editor_replacement() {
    const HMODULE rich_edit = LoadLibraryW(L"Msftedit.dll");
    expect(rich_edit != nullptr, "Windows RichEdit library must load");
    for (const auto* class_name : {L"EDIT", L"RICHEDIT50W"}) {
        test_focus = CreateWindowExW(0, class_name, L"", WS_POPUP | ES_MULTILINE |
                                      ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                                      0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr);
        expect(test_focus != nullptr, "hidden native edit control must exist");
        if (test_focus == nullptr) { continue; }
        accepted_count = (std::numeric_limits<UINT>::max)();
        expect(send_edit({L'é', false}) == SendStatus::success, "native append succeeds");
        expect(send_edit({L'è', true}) == SendStatus::success, "native cycle succeeds");
        wchar_t text[32]{};
        GetWindowTextW(test_focus, text, 32);
        expect(std::wstring(text) == L"è", "native replacement must change real control text");

        SendMessageW(test_focus, EM_SETSEL, 0, 0);
        expect(send_edit({L'ê', true}) != SendStatus::success,
               "moved caret must reject stale native replacement");
        GetWindowTextW(test_focus, text, 32);
        expect(std::wstring(text) == L"è", "stale replacement must not delete or insert text");

        SendMessageW(test_focus, EM_SETREADONLY, TRUE, 0);
        expect(send_edit({L'é', false}) == SendStatus::blocked,
               "read-only native control must fail open without editing");
        SendMessageW(test_focus, EM_SETREADONLY, FALSE, 0);

        SetWindowTextW(test_focus, L"x");
        SendMessageW(test_focus, EM_SETSEL, 1, 1);
        SendMessageW(test_focus, EM_LIMITTEXT, 1, 0);
        if (wcscmp(class_name, L"RICHEDIT50W") == 0) {
            SendMessageW(test_focus, EM_EXLIMITTEXT, 0, 1);
        }
        const auto limited_status = send_edit({L'é', false});
        if (limited_status == SendStatus::success) {
            std::wcerr << L"text-limit diagnostic: " << class_name
                       << L" length=" << GetWindowTextLengthW(test_focus) << L'\n';
        }
        expect(limited_status != SendStatus::success,
               "native text limit must not be mistaken for successful insertion");

        SendMessageW(test_focus, EM_LIMITTEXT, 100000, 0);
        if (wcscmp(class_name, L"RICHEDIT50W") == 0) {
            SendMessageW(test_focus, EM_EXLIMITTEXT, 0, 100000);
        }
        const std::wstring prefix(70000, L'x');
        SetWindowTextW(test_focus, prefix.c_str());
        SendMessageW(test_focus, EM_SETSEL, 70000, 70000);
        expect(send_edit({L'é', false}) == SendStatus::success, "append beyond 16-bit position");
        expect(send_edit({L'è', true}) == SendStatus::success, "cycle beyond 16-bit position");
        std::wstring result(70002, L'\0');
        const int length = GetWindowTextW(test_focus, result.data(), static_cast<int>(result.size()));
        result.resize(static_cast<std::size_t>(length));
        if (result != prefix + L"è") {
            std::wcerr << L"native long-document diagnostic: " << class_name
                       << L" length=" << result.size() << L" last="
                       << (result.empty() ? 0U : static_cast<unsigned>(result.back())) << L'\n';
        }
        expect(result == prefix + L"è", "full 32-bit selection must preserve the long prefix");
        DestroyWindow(test_focus);
        test_focus = nullptr;
    }
    if (rich_edit != nullptr) { FreeLibrary(rich_edit); }
}
}

int main() {
    output_contract();
    native_editor_replacement();
    if (failures != 0) {
        return EXIT_FAILURE;
    }
    std::cout << "Win32 output contract passed (SendInput mocked; no desktop input)\n";
    return EXIT_SUCCESS;
}
