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
