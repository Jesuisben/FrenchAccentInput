#include <windows.h>

#include <cstdlib>
#include <iostream>
#include <string>

// Opt-in integration driver. Only fixed test input is sent to a caller-selected
// foreground window; it never activates windows or records user input.
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "Usage: fai_virtual_driver HWND e-cycle|mapping|fast-mapping|typematic|direct|direct-paced\n";
        return 2;
    }
    const HWND target = reinterpret_cast<HWND>(std::wcstoull(argv[1], nullptr, 10));
    const std::wstring mode = argv[2];
    if (mode == L"inspect" && IsWindow(target)) {
        GUITHREADINFO info{};
        info.cbSize = sizeof(info);
        if (!GetGUIThreadInfo(GetWindowThreadProcessId(target, nullptr), &info)) { return 1; }
        wchar_t focus_class[128]{};
        GetClassNameW(info.hwndFocus, focus_class, 128);
        std::wcout << L"focus class=" << focus_class << L"; focus="
                   << reinterpret_cast<std::uintptr_t>(info.hwndFocus)
                   << L"; caret=" << reinterpret_cast<std::uintptr_t>(info.hwndCaret) << L'\n';
        return 0;
    }
    if (!IsWindow(target) || (mode != L"e-cycle" && mode != L"mapping" &&
                              mode != L"fast-mapping" && mode != L"typematic" && mode != L"direct" &&
                              mode != L"direct-paced" && mode != L"direct-scan")) {
        return 2;
    }
    for (const int key : {VK_MENU, VK_CONTROL, VK_SHIFT, VK_LWIN, VK_RWIN}) {
        if ((GetAsyncKeyState(key) & 0x8000) != 0) {
            std::cerr << "Refused: modifier already held\n";
            return 3;
        }
    }
    if (mode == L"direct" || mode == L"direct-paced" || mode == L"direct-scan") {
        // Same fixed replacement workload, without Alt, router or hook output.
        for (const std::wstring values : {L"àâæ", L"ç", L"ç", L"ç", L"éèêë",
                                          L"ïî", L"ôœ", L"ùûü", L"ÿ", L"ÿ", L"ÿ"}) {
            for (std::size_t index = 0; index < values.size(); ++index) {
                if (GetForegroundWindow() != target) { return 3; }
                INPUT inputs[4]{};
                UINT count = 0;
                if (index != 0) {
                    inputs[count].type = INPUT_KEYBOARD;
                    inputs[count++].ki.wVk = VK_BACK;
                    if (mode == L"direct-scan") {
                        inputs[count - 1].ki.wVk = 0;
                        inputs[count - 1].ki.wScan = 0x0E;
                        inputs[count - 1].ki.dwFlags = KEYEVENTF_SCANCODE;
                    }
                    inputs[count] = inputs[count - 1];
                    inputs[count++].ki.dwFlags |= KEYEVENTF_KEYUP;
                }
                inputs[count].type = INPUT_KEYBOARD;
                inputs[count].ki.wScan = static_cast<WORD>(values[index]);
                inputs[count++].ki.dwFlags = KEYEVENTF_UNICODE;
                inputs[count] = inputs[count - 1];
                inputs[count++].ki.dwFlags |= KEYEVENTF_KEYUP;
                for (UINT item = 0; item < count; ++item) {
                    inputs[item].ki.dwExtraInfo = 0x46414932;
                }
                if (mode == L"direct-paced") {
                    for (UINT item = 0; item < count; ++item) {
                        if (GetForegroundWindow() != target) { return 3; }
                        if (SendInput(1, &inputs[item], sizeof(INPUT)) != 1) { return 1; }
                        Sleep(10);
                    }
                } else if (SendInput(count, inputs, sizeof(INPUT)) != count) { return 1; }
            }
        }
        Sleep(150);
        std::cout << "Direct Unicode/Backspace workload delivered (no Alt)\n";
        return 0;
    }
    bool alt_down = false;
    auto send = [&](WORD key, bool down) {
        if (GetForegroundWindow() != target) {
            return false;
        }
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = key;
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
        input.ki.dwExtraInfo = 0x46414932;
        if (SendInput(1, &input, sizeof(input)) != 1) {
            return false;
        }
        if (key == VK_LMENU) { alt_down = down; }
        if (mode != L"fast-mapping") { Sleep(60); }
        return true;
    };
    bool ok = send(VK_LMENU, true);
    const std::wstring sequence = (mode == L"e-cycle" || mode == L"typematic")
                                     ? L"EEEE" : L"AAACCCEEEEIIOOUUUYYY";
    for (const wchar_t key : sequence) {
        if (!ok) { break; }
        ok = send(static_cast<WORD>(key), true);
        if (ok && mode != L"typematic") { ok = send(static_cast<WORD>(key), false); }
    }
    if (ok && mode == L"typematic") { ok = send('E', false); }
    if (alt_down) {
        // Always release our Alt, including after a focus loss. No text is sent
        // once the selected target has lost foreground ownership.
        INPUT release{};
        release.type = INPUT_KEYBOARD;
        release.ki.wVk = VK_LMENU;
        release.ki.dwFlags = KEYEVENTF_KEYUP;
        release.ki.dwExtraInfo = 0x46414932;
        ok = SendInput(1, &release, sizeof(release)) == 1 && ok;
    }
    Sleep(150);
    bool released = true;
    for (const int key : {VK_MENU, VK_CONTROL, VK_SHIFT}) {
        released = released && (GetAsyncKeyState(key) & 0x8000) == 0;
    }
    std::cout << "Fixed synthetic sequence delivered=" << ok
              << "; modifiers released=" << released
              << "; target retained=" << (GetForegroundWindow() == target) << '\n';
    return ok && released && GetForegroundWindow() == target ? 0 : 1;
}
