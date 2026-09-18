#include "fai/input_core.h"

#include <windows.h>
#include <shellapi.h>

#include <array>
#include <cstdint>
#include <cwchar>
#include <vector>

namespace {

#if defined(FAI_TEST_ALLOW_INJECTED)
constexpr wchar_t app_name[] = L"French Accent Input Virtual Keyboard Test";
constexpr wchar_t window_class_name[] = L"FrenchAccentInput.VirtualKeyboardTest.HiddenWindow";
constexpr wchar_t mutex_name[] = L"Local\\FrenchAccentInput-VirtualKeyboardTest";
#else
constexpr wchar_t app_name[] = L"French Accent Input";
constexpr wchar_t window_class_name[] = L"FrenchAccentInput.HiddenWindow";
constexpr wchar_t mutex_name[] = L"Local\\FrenchAccentInput-4A67FCE1-5DC0-4ACB-9843-9672E4CBE071";
#endif
constexpr ULONG_PTR injection_marker = 0x46414931;
// 0xE8 is unassigned by Windows. A non-modifier cancels modern access-key mode;
// Ctrl alone did not cancel it in Windows 11 Notepad integration tests.
constexpr WORD menu_mask_key = 0xE8;
constexpr UINT tray_icon_id = 1;
constexpr UINT tray_callback_message = WM_APP + 1;
constexpr UINT input_warning_message = WM_APP + 2;
constexpr UINT virtual_keyboard_message = WM_APP + 3;
constexpr UINT exit_command_id = 1001;

HHOOK keyboard_hook = nullptr;
HHOOK mouse_hook = nullptr;
HWND hidden_window = nullptr;
UINT taskbar_created_message = 0;
fai::InputRouter router;
bool warning_shown = false;
int integration_exit_code = 0;
bool integration_direct_unicode = false;

#if defined(FAI_TEST_ALLOW_INJECTED)
constexpr bool allow_virtual_keyboard_input = true;
#else
constexpr bool allow_virtual_keyboard_input = false;
#endif

INPUT unicode_input(wchar_t character, DWORD flags) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = static_cast<WORD>(character);
    input.ki.dwFlags = KEYEVENTF_UNICODE | flags;
    input.ki.dwExtraInfo = injection_marker;
    return input;
}

INPUT virtual_key_input(WORD virtual_key, DWORD flags) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = virtual_key;
    input.ki.dwFlags = flags;
    return input;
}

INPUT marked_virtual_key_input(WORD virtual_key, DWORD flags) {
    auto input = virtual_key_input(virtual_key, flags);
    input.ki.dwExtraInfo = injection_marker;
    return input;
}

void append_masked_alt_up(std::vector<INPUT>& inputs) {
    // The mask supplies an intervening non-modifier before releasing Alt.
    inputs.push_back(marked_virtual_key_input(menu_mask_key, 0));
    inputs.push_back(marked_virtual_key_input(VK_LMENU, KEYEVENTF_KEYUP));
    inputs.push_back(marked_virtual_key_input(menu_mask_key, KEYEVENTF_KEYUP));
}

bool send_virtual_e_cycle() {
    std::array<INPUT, 10> inputs{
        virtual_key_input(VK_LMENU, 0),
        virtual_key_input('E', 0),
        virtual_key_input('E', KEYEVENTF_KEYUP),
        virtual_key_input('E', 0),
        virtual_key_input('E', KEYEVENTF_KEYUP),
        virtual_key_input('E', 0),
        virtual_key_input('E', KEYEVENTF_KEYUP),
        virtual_key_input('E', 0),
        virtual_key_input('E', KEYEVENTF_KEYUP),
        virtual_key_input(VK_LMENU, KEYEVENTF_KEYUP),
    };
    return SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) == inputs.size();
}

enum class SendStatus { success, blocked, partial, stale };

void recover_partial_input(const INPUT* inputs, UINT sent, bool hold_alt) {
    std::vector<INPUT> recovery;
    bool alt_down = true;
    for (UINT i = 0; i < sent; ++i) {
        const auto& key = inputs[i].ki;
        if (key.wVk == VK_LMENU) {
            alt_down = (key.dwFlags & KEYEVENTF_KEYUP) == 0;
            continue;
        }
        if ((key.dwFlags & KEYEVENTF_KEYUP) != 0) { continue; }
        bool released = false;
        for (UINT j = i + 1; j < sent; ++j) {
            const auto& next = inputs[j].ki;
            if (next.wVk == key.wVk && next.wScan == key.wScan &&
                (next.dwFlags & KEYEVENTF_KEYUP) != 0) {
                released = true;
                break;
            }
        }
        if (!released) {
            auto up = inputs[i];
            up.ki.dwFlags |= KEYEVENTF_KEYUP;
            recovery.push_back(up);
        }
    }
    if (alt_down != hold_alt) {
        recovery.push_back(marked_virtual_key_input(VK_LMENU, hold_alt ? 0 : KEYEVENTF_KEYUP));
    }
    // Best effort only: Windows may also block recovery. Never replay text or
    // Backspace, and keep the original failure visible to the caller.
    if (!recovery.empty()) {
        (void)SendInput(static_cast<UINT>(recovery.size()), recovery.data(), sizeof(INPUT));
    }
}

HWND native_edit_control = nullptr;
DWORD native_edit_caret = 0;

bool native_selection(HWND control, DWORD& start, DWORD& end) {
    DWORD_PTR result = 0;
    return SendMessageTimeoutW(control, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                               reinterpret_cast<LPARAM>(&end), SMTO_ABORTIFHUNG | SMTO_BLOCK,
                               50, &result) != 0;
}

std::optional<SendStatus> send_native_edit(const fai::Edit& edit, std::vector<INPUT>& inputs) {
    const HWND foreground = GetForegroundWindow();
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    wchar_t class_name[128]{};
    if (!GetGUIThreadInfo(GetWindowThreadProcessId(foreground, nullptr), &info) ||
        GetClassNameW(info.hwndFocus, class_name, 128) == 0 ||
        (_wcsicmp(class_name, L"EDIT") != 0 && _wcsnicmp(class_name, L"RichEdit", 8) != 0)) {
        native_edit_control = nullptr;
        return std::nullopt;
    }
    const HWND control = info.hwndFocus;
    DWORD start = 0;
    DWORD end = 0;
    if ((GetWindowLongPtrW(control, GWL_STYLE) & ES_READONLY) != 0 ||
        !IsWindowEnabled(control) || !native_selection(control, start, end)) {
        return SendStatus::blocked;
    }
    if (edit.replace_previous && (control != native_edit_control || start != end ||
                                   end != native_edit_caret || start == 0)) {
        native_edit_control = nullptr;
        return SendStatus::stale;
    }
    if (!edit.replace_previous && start == end) {
        DWORD_PTR limit = 0;
        DWORD_PTR length = 0;
        // EM_REPLACESEL can bypass RichEdit's typing limit. Honor that limit
        // without reading or retaining the document's contents.
        if (!SendMessageTimeoutW(control, EM_GETLIMITTEXT, 0, 0,
                                 SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &limit) ||
            !SendMessageTimeoutW(control, WM_GETTEXTLENGTH, 0, 0,
                                 SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &length) || length >= limit) {
            return SendStatus::blocked;
        }
    }
    if (GetForegroundWindow() != foreground) { return SendStatus::blocked; }
    const DWORD expected_caret = edit.replace_previous ? start : start + 1;

    // RichEdit's keyboard/Unicode paths can overtake one another under load.
    // Its native selection replacement is synchronous and needs no timing delay.
    const UINT released = SendInput(3, inputs.data(), sizeof(INPUT));
    if (released != 3) {
        if (released != 0) { recover_partial_input(inputs.data(), released, true); }
        return released == 0 ? SendStatus::blocked : SendStatus::partial;
    }
    DWORD_PTR ignored = 0;
    bool selected = true;
    if (edit.replace_previous) {
        selected = SendMessageTimeoutW(control, EM_SETSEL, start - 1, end,
                                       SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &ignored) != 0;
    }
    const wchar_t text[] = {edit.character, L'\0'};
    const bool edited = selected && SendMessageTimeoutW(control, EM_REPLACESEL, TRUE,
                         reinterpret_cast<LPARAM>(text), SMTO_ABORTIFHUNG | SMTO_BLOCK,
                         50, &ignored) != 0;
    if (!edited && selected && edit.replace_previous) {
        (void)SendMessageTimeoutW(control, EM_SETSEL, start, end,
                                 SMTO_ABORTIFHUNG | SMTO_BLOCK, 50, &ignored);
    }
    native_edit_control = nullptr;
    const bool positioned = edited && native_selection(control, start, end) &&
                            start == end && end == expected_caret;
    if (positioned) {
        native_edit_control = control;
        native_edit_caret = end;
    }
    const UINT restored = SendInput(1, &inputs.back(), sizeof(INPUT));
    if (restored != 1) { recover_partial_input(inputs.data(), 3, true); }
    return positioned && restored == 1 ? SendStatus::success : SendStatus::partial;
}

SendStatus send_edit(const fai::Edit& edit) {
    std::vector<INPUT> inputs;
    inputs.reserve((edit.replace_previous ? 4U : 2U) + 5U);

    append_masked_alt_up(inputs);

    if (edit.replace_previous) {
        // U+0008 via VK_PACKET is not a Backspace key in modern text controls.
        inputs.push_back(marked_virtual_key_input(VK_BACK, 0));
        inputs.push_back(marked_virtual_key_input(VK_BACK, KEYEVENTF_KEYUP));
    }

    // Alt is temporarily released above and restored below for native shortcuts.
    // KEYEVENTF_UNICODE는 현재 keyboard layout과 무관하게 UTF-16 문자를 전달한다.
    inputs.push_back(unicode_input(edit.character, 0));
    inputs.push_back(unicode_input(edit.character, KEYEVENTF_KEYUP));

    inputs.push_back(marked_virtual_key_input(VK_LMENU, 0));

    if (const auto native_status = send_native_edit(edit, inputs)) {
        return *native_status;
    }
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent == inputs.size()) {
        return SendStatus::success;
    }
    if (sent == 0) {
        return SendStatus::blocked;
    }

    recover_partial_input(inputs.data(), sent, true);
    return SendStatus::partial;
}

SendStatus send_masked_alt_release() {
    std::vector<INPUT> inputs;
    inputs.reserve(3U);
    append_masked_alt_up(inputs);
    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != 0 && sent != inputs.size()) {
        recover_partial_input(inputs.data(), sent, false);
    }
    return sent == inputs.size() ? SendStatus::success : (sent == 0 ? SendStatus::blocked : SendStatus::partial);
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

    const bool injected = data->dwExtraInfo == injection_marker ||
                          (!allow_virtual_keyboard_input && (data->flags & LLKHF_INJECTED) != 0);
    if (injected) {
        // Other software can type or move the caret. Never replace across it.
        // Our own output must leave the active cycle intact and cannot recurse.
        if (data->dwExtraInfo != injection_marker) {
            router.cancel_sequence();
        }
        return CallNextHookEx(keyboard_hook, code, message, event_data);
    }
    const HWND foreground = GetForegroundWindow();
    GUITHREADINFO focus{};
    focus.cbSize = sizeof(focus);
    const bool has_focus = GetGUIThreadInfo(GetWindowThreadProcessId(foreground, nullptr), &focus) &&
                           focus.hwndFocus != nullptr;
    const fai::KeyEvent event{
        .virtual_key = data->vkCode,
        .key_down = key_down,
        .injected = injected,
        .target = reinterpret_cast<std::uintptr_t>(has_focus ? focus.hwndFocus : foreground),
    };
    auto route = router.handle(event);

    if (route.edit) {
        // A modifier can predate hook installation or come from other software.
        // Query only other keys: the current callback's key state is not updated yet.
        for (const int modifier : {VK_LCONTROL, VK_RCONTROL, VK_LSHIFT, VK_RSHIFT,
                                   VK_LWIN, VK_RWIN, VK_RMENU}) {
            if ((GetAsyncKeyState(modifier) & 0x8000) != 0) {
                router.abort_consumed_key(data->vkCode);
                return CallNextHookEx(keyboard_hook, code, message, event_data);
            }
        }
        auto status = send_edit(*route.edit);
        if (status == SendStatus::stale) {
            router.cancel_sequence();
            route = router.handle(event);
            status = route.edit ? send_edit(*route.edit) : SendStatus::blocked;
        }
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

    if (route.mask_left_alt_release) {
        const auto status = send_masked_alt_release();
        if (status != SendStatus::success) {
            return CallNextHookEx(keyboard_hook, code, message, event_data);
        }
    }

    if (route.suppress) {
        return 1;
    }
    return CallNextHookEx(keyboard_hook, code, message, event_data);
}

LRESULT CALLBACK mouse_proc(int code, WPARAM message, LPARAM event_data) {
    if (code >= 0) {
        const bool changes_caret = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                                   message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN ||
                                   message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
        if (changes_caret) {
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
    case virtual_keyboard_message:
        if (!allow_virtual_keyboard_input) {
            integration_exit_code = 1;
        } else if (integration_direct_unicode) {
            std::array<INPUT, 2> inputs{
                unicode_input(L'é', 0),
                unicode_input(L'é', KEYEVENTF_KEYUP),
            };
            if (SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) !=
                inputs.size()) {
                integration_exit_code = 1;
            }
        } else if (!send_virtual_e_cycle()) {
            integration_exit_code = 1;
        }
        DestroyWindow(window);
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

    if (allow_virtual_keyboard_input && command_line != nullptr &&
        (wcsstr(command_line, L"--virtual-e-cycle") != nullptr ||
         wcsstr(command_line, L"--virtual-e-cycle-delayed") != nullptr ||
         wcsstr(command_line, L"--virtual-unicode-e") != nullptr)) {
        integration_direct_unicode = wcsstr(command_line, L"--virtual-unicode-e") != nullptr;
        if (wcsstr(command_line, L"--virtual-e-cycle-delayed") != nullptr) {
            Sleep(5000);
        }
        (void)PostMessageW(hidden_window, virtual_keyboard_message, 0, 0);
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
    return integration_exit_code != 0 ? integration_exit_code : static_cast<int>(message.wParam);
}
