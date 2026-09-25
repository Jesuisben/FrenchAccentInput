#include "fai/input_core.h"
#include "../resources/resource_ids.h"

#include <windows.h>
#include <shellapi.h>
#include <imm.h>

#include <array>
#include <cstdint>
#include <cwchar>
#include <string>
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
constexpr UINT open_usage_message = WM_APP + 4;
constexpr UINT usage_command_id = 1002;
constexpr UINT settings_command_id = 1003;
constexpr UINT about_command_id = 1004;
constexpr UINT exit_command_id = 1001;
constexpr UINT startup_checkbox_id = 2001;
constexpr wchar_t usage_class_name[] = L"FrenchAccentInput.UsageWindow";
constexpr wchar_t settings_class_name[] = L"FrenchAccentInput.SettingsWindow";
constexpr wchar_t startup_key[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t startup_value[] = L"FrenchAccentInput";

enum class StartupAction { normal, autostart, enable_autostart, disable_autostart, quit_existing };

StartupAction startup_action(const wchar_t* command_line) {
    if (command_line == nullptr) { return StartupAction::normal; }
    if (wcscmp(command_line, L"--autostart") == 0) { return StartupAction::autostart; }
    if (wcscmp(command_line, L"--enable-autostart") == 0) { return StartupAction::enable_autostart; }
    if (wcscmp(command_line, L"--disable-autostart") == 0) { return StartupAction::disable_autostart; }
    if (wcscmp(command_line, L"--quit-existing") == 0) { return StartupAction::quit_existing; }
    return StartupAction::normal;
}

HHOOK keyboard_hook = nullptr;
HHOOK mouse_hook = nullptr;
HWND hidden_window = nullptr;
HWND usage_window = nullptr;
HWND settings_window = nullptr;
HINSTANCE app_instance = nullptr;
HFONT usage_heading_font = nullptr;
HFONT usage_section_font = nullptr;
HFONT usage_body_font = nullptr;
HFONT usage_accent_font = nullptr;
UINT taskbar_created_message = 0;
fai::InputRouter router;
bool warning_shown = false;
int integration_exit_code = 0;
bool integration_direct_unicode = false;
bool caps_lock_on = false;
bool caps_key_down = false;

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

std::wstring own_startup_command() {
    std::array<wchar_t, MAX_PATH> path{};
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) { return {}; }
    return L"\"" + std::wstring(path.data(), length) + L"\" --autostart";
}

LONG read_startup_command(std::wstring& command) {
    std::array<wchar_t, 512> value{};
    DWORD bytes = sizeof(value);
    const LONG result = RegGetValueW(HKEY_CURRENT_USER, startup_key, startup_value,
                                     RRF_RT_REG_SZ, nullptr, value.data(), &bytes);
    if (result == ERROR_SUCCESS) { command = value.data(); }
    return result;
}

bool autostart_enabled() {
    std::wstring existing;
    const auto wanted = own_startup_command();
    return !wanted.empty() && read_startup_command(existing) == ERROR_SUCCESS &&
           _wcsicmp(existing.c_str(), wanted.c_str()) == 0;
}

bool set_autostart(bool enabled) {
    const auto wanted = own_startup_command();
    if (wanted.empty() || wanted.size() > 259) { return false; }
    if (enabled) {
        return RegSetKeyValueW(HKEY_CURRENT_USER, startup_key, startup_value, REG_SZ,
                               wanted.c_str(), static_cast<DWORD>((wanted.size() + 1) * sizeof(wchar_t))) ==
               ERROR_SUCCESS;
    }
    std::wstring existing;
    const LONG result = read_startup_command(existing);
    if (result == ERROR_FILE_NOT_FOUND) { return true; }
    if (result != ERROR_SUCCESS) { return false; }
    // Never remove a different installation's login entry.
    return _wcsicmp(existing.c_str(), wanted.c_str()) != 0 ||
           RegDeleteKeyValueW(HKEY_CURRENT_USER, startup_key, startup_value) == ERROR_SUCCESS;
}

void show_launch_notification() {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hidden_window;
    data.uID = tray_icon_id;
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIIF_INFO;
    wcscpy_s(data.szInfoTitle, L"French Accent Input");
    wcscpy_s(data.szInfo, L"실행 중입니다. 알림 영역 아이콘에서 사용법과 설정을 열 수 있습니다.");
    (void)Shell_NotifyIconW(NIM_MODIFY, &data);
}

bool add_tray_icon() {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hidden_window;
    data.uID = tray_icon_id;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = tray_callback_message;
    data.hIcon = static_cast<HICON>(LoadImageW(app_instance, MAKEINTRESOURCEW(IDI_FAI_APP),
                                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
    if (data.hIcon == nullptr) { return false; }
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

void activate_window(HWND window) {
    if (IsIconic(window)) { ShowWindow(window, SW_RESTORE); }
    else { ShowWindow(window, SW_SHOW); }
    (void)SetForegroundWindow(window);
}

void usage_label(HWND parent, const wchar_t* value, int x, int y, int width, int height,
                 HFONT font) {
    const HWND label = CreateWindowExW(0, L"STATIC", value, WS_CHILD | WS_VISIBLE,
                                       x, y, width, height, parent, nullptr, app_instance, nullptr);
    if (label != nullptr) {
        SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

LRESULT CALLBACK usage_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_CREATE) {
        if (usage_heading_font == nullptr) {
            usage_heading_font = CreateFontW(-23, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                             DEFAULT_PITCH, L"Segoe UI");
            usage_section_font = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                             DEFAULT_PITCH, L"Segoe UI");
            usage_body_font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH, L"Segoe UI");
            usage_accent_font = CreateFontW(-20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                            DEFAULT_PITCH, L"Segoe UI");
        }
        const HFONT heading = usage_heading_font != nullptr ? usage_heading_font :
                              static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        const HFONT section = usage_section_font != nullptr ? usage_section_font : heading;
        const HFONT body = usage_body_font != nullptr ? usage_body_font : heading;
        const HFONT accent = usage_accent_font != nullptr ? usage_accent_font : body;
        constexpr int margin = 24;
        constexpr int case_alt_x = 144;
        constexpr int case_shift_x = 369;
        constexpr int key_x = 36;
        constexpr int lower_x = 145;
        constexpr int upper_x = 360;
        constexpr int case_row_height = 23;
        constexpr int character_row_height = 28;
        int y = 16;

        usage_label(window, L"프랑스어 특수 문자 입력", margin, y, 560, 29, heading);
        y += 31;
        usage_label(window, L"왼쪽 Alt를 누른 채 글자 키를 누르면 프랑스어 특수 문자가 입력됩니다.",
                    margin, y, 560, 24, body);
        y += 28;
        usage_label(window, L"대소문자 입력", margin, y, 560, 24, section);
        y += 27;
        usage_label(window, L"Caps Lock", margin, y, 110, 22, section);
        usage_label(window, L"왼쪽 Alt + 글자", case_alt_x, y, 215, 22, section);
        usage_label(window, L"Shift + 왼쪽 Alt + 글자", case_shift_x, y, 225, 22, section);
        y += case_row_height;
        usage_label(window, L"꺼짐", margin, y, 110, 22, body);
        usage_label(window, L"소문자 프랑스어 특수 문자", case_alt_x, y, 215, 22, body);
        usage_label(window, L"대문자 프랑스어 특수 문자", case_shift_x, y, 225, 22, body);
        y += case_row_height;
        usage_label(window, L"켜짐", margin, y, 110, 22, body);
        usage_label(window, L"대문자 프랑스어 특수 문자", case_alt_x, y, 215, 22, body);
        usage_label(window, L"소문자 프랑스어 특수 문자", case_shift_x, y, 225, 22, body);
        y += case_row_height + 7;

        usage_label(window, L"지원 문자", margin, y, 560, 24, section);
        y += 26;
        usage_label(window, L"키", key_x, y, 70, 22, section);
        usage_label(window, L"소문자", lower_x, y, 190, 22, section);
        usage_label(window, L"대문자", upper_x, y, 210, 22, section);
        y += 24;
        struct Row { const wchar_t* shortcut; const wchar_t* lower; const wchar_t* upper; };
        constexpr Row rows[] = {
            {L"A", L"à → â → æ", L"À → Â → Æ"},
            {L"C", L"ç", L"Ç"},
            {L"E", L"é → è → ê → ë", L"É → È → Ê → Ë"},
            {L"I", L"ï → î", L"Ï → Î"},
            {L"O", L"ô → œ", L"Ô → Œ"},
            {L"U", L"ù → û → ü", L"Ù → Û → Ü"},
            {L"Y", L"ÿ", L"Ÿ"},
        };
        for (const auto& row : rows) {
            usage_label(window, row.shortcut, key_x, y, 70, 26, section);
            usage_label(window, row.lower, lower_x, y, 190, 26, accent);
            usage_label(window, row.upper, upper_x, y, 210, 26, accent);
            y += character_row_height;
        }
        y += 7;
        usage_label(window, L"사용 팁", margin, y, 560, 24, section);
        y += 26;
        usage_label(window, L"같은 키 반복 → 다음 문자로 순환", margin, y, 290, 22, body);
        usage_label(window, L"C, Y → 누를 때마다 새 문자 추가", 320, y, 274, 22, body);
        y += 24;
        usage_label(window, L"X : 창만 닫기·입력기는 계속 실행", margin, y, 290, 22, body);
        usage_label(window, L"종료: 알림 영역 우클릭 → 종료", 320, y, 274, 22, body);
        return 0;
    }
    if (message == WM_CTLCOLORSTATIC) {
        SetBkMode(reinterpret_cast<HDC>(wparam), TRANSPARENT);
        return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
    }
    if (message == WM_DESTROY) { usage_window = nullptr; return 0; }
    return DefWindowProcW(window, message, wparam, lparam);
}

void show_usage() {
    if (usage_window != nullptr) { activate_window(usage_window); return; }
    usage_window = CreateWindowExW(WS_EX_APPWINDOW, usage_class_name,
                                   L"French Accent Input - 사용법",
                                   WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 620, 560,
                                   nullptr, nullptr, app_instance, nullptr);
    if (usage_window != nullptr) { activate_window(usage_window); }
}

LRESULT CALLBACK settings_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_CREATE) {
        const HWND checkbox = CreateWindowExW(0, L"BUTTON", L"Windows 로그인 시 자동 실행",
                                               WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                               20, 22, 310, 28, window,
                                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(startup_checkbox_id)),
                                               app_instance, nullptr);
        if (checkbox != nullptr) {
            SendMessageW(checkbox, WM_SETFONT,
                         reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
            SendMessageW(checkbox, BM_SETCHECK, autostart_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        const HWND note = CreateWindowExW(0, L"STATIC", L"변경 즉시 현재 사용자 계정에 적용됩니다.",
                                           WS_CHILD | WS_VISIBLE, 20, 61, 310, 25,
                                           window, nullptr, app_instance, nullptr);
        if (note != nullptr) {
            SendMessageW(note, WM_SETFONT,
                         reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        }
        return 0;
    }
    if (message == WM_COMMAND && LOWORD(wparam) == startup_checkbox_id &&
        HIWORD(wparam) == BN_CLICKED) {
        const HWND checkbox = reinterpret_cast<HWND>(lparam);
        const bool enabled = SendMessageW(checkbox, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (!set_autostart(enabled)) {
            SendMessageW(checkbox, BM_SETCHECK, enabled ? BST_UNCHECKED : BST_CHECKED, 0);
            MessageBoxW(window, L"자동 실행 설정을 변경하지 못했습니다.", app_name,
                        MB_OK | MB_ICONERROR);
        }
        return 0;
    }
    if (message == WM_DESTROY) { settings_window = nullptr; return 0; }
    return DefWindowProcW(window, message, wparam, lparam);
}

void show_settings() {
    if (settings_window != nullptr) { activate_window(settings_window); return; }
    settings_window = CreateWindowExW(WS_EX_APPWINDOW, settings_class_name,
                                      L"French Accent Input - 설정",
                                      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 380, 155,
                                      nullptr, nullptr, app_instance, nullptr);
    if (settings_window != nullptr) { activate_window(settings_window); }
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
    (void)AppendMenuW(menu, MF_STRING, usage_command_id, L"사용법");
    (void)AppendMenuW(menu, MF_STRING, settings_command_id, L"설정");
    (void)AppendMenuW(menu, MF_STRING, about_command_id, L"정보");
    (void)AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    (void)AppendMenuW(menu, MF_STRING, exit_command_id, L"종료");
    (void)SetForegroundWindow(hidden_window);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        point.x, point.y, 0, hidden_window, nullptr);
    DestroyMenu(menu);
    if (command == usage_command_id) { show_usage(); }
    if (command == settings_command_id) { show_settings(); }
    if (command == about_command_id) {
        MessageBoxW(usage_window != nullptr ? usage_window : hidden_window,
                    L"French Accent Input\r\nVersion 1.1.0", L"French Accent Input - 정보",
                    MB_OK | MB_ICONINFORMATION);
    }
    if (command == exit_command_id) { DestroyWindow(hidden_window); }
}

bool korean_hangul_mode(HWND focus) {
    const DWORD thread = GetWindowThreadProcessId(focus, nullptr);
    if (thread == 0 || LOWORD(reinterpret_cast<ULONG_PTR>(GetKeyboardLayout(thread))) != 0x0412) {
        return false;
    }
    const HWND ime_window = ImmGetDefaultIMEWnd(focus);
    DWORD_PTR conversion = 0;
    // A failed query is treated as Hangul: never inject Latin text into an unknown Korean mode.
    return ime_window == nullptr ||
           !SendMessageTimeoutW(ime_window, WM_IME_CONTROL, 0x0001, 0,
                                SMTO_ABORTIFHUNG | SMTO_BLOCK, 20, &conversion) ||
           (conversion & IME_CMODE_NATIVE) != 0;
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

    if (data->vkCode == VK_CAPITAL) {
        if (key_down && !caps_key_down) {
            caps_lock_on = !caps_lock_on;
            caps_key_down = true;
        } else if (key_up) {
            caps_key_down = false;
        }
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
    const HWND target = has_focus ? focus.hwndFocus : foreground;
    const bool accent_key = data->vkCode == 'A' || data->vkCode == 'C' ||
                            data->vkCode == 'E' || data->vkCode == 'I' ||
                            data->vkCode == 'O' || data->vkCode == 'U' ||
                            data->vkCode == 'Y';
    const bool accent_allowed = !(key_down && accent_key && (data->flags & LLKHF_ALTDOWN) &&
                                  korean_hangul_mode(target));
    const fai::KeyEvent event{
        .virtual_key = data->vkCode,
        .key_down = key_down,
        .injected = injected,
        .target = reinterpret_cast<std::uintptr_t>(target),
        .accent_allowed = accent_allowed,
        .caps_lock_on = caps_lock_on,
        .shift_down = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0 ||
                      (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0,
    };
    auto route = router.handle(event);

    if (route.edit) {
        // A modifier can predate hook installation or come from other software.
        // Query only other keys: the current callback's key state is not updated yet.
        for (const int modifier : {VK_LCONTROL, VK_RCONTROL, VK_LWIN, VK_RWIN, VK_RMENU}) {
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
        if (LOWORD(lparam) == NIN_SELECT || LOWORD(lparam) == NIN_KEYSELECT ||
            LOWORD(lparam) == WM_LBUTTONUP) {
            show_usage();
        }
        if (LOWORD(lparam) == WM_RBUTTONUP || LOWORD(lparam) == WM_CONTEXTMENU) {
            show_tray_menu();
        }
        return 0;
    case open_usage_message:
        show_usage();
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
        if (usage_window != nullptr) { DestroyWindow(usage_window); }
        if (settings_window != nullptr) { DestroyWindow(settings_window); }
        if (usage_heading_font != nullptr) { DeleteObject(usage_heading_font); }
        if (usage_section_font != nullptr) { DeleteObject(usage_section_font); }
        if (usage_body_font != nullptr) { DeleteObject(usage_body_font); }
        if (usage_accent_font != nullptr) { DeleteObject(usage_accent_font); }
        usage_heading_font = usage_section_font = usage_body_font = usage_accent_font = nullptr;
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
    const StartupAction action = startup_action(command_line);
    if (action == StartupAction::enable_autostart || action == StartupAction::disable_autostart) {
        if (set_autostart(action == StartupAction::enable_autostart)) { return 0; }
        MessageBoxW(nullptr, L"Windows 로그인 시 자동 실행 설정을 변경하지 못했습니다.",
                    app_name, MB_OK | MB_ICONERROR);
        return 1;
    }
    if (action == StartupAction::quit_existing) {
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
        if (action != StartupAction::autostart) {
            HWND existing = nullptr;
            for (int attempt = 0; attempt < 20 && existing == nullptr; ++attempt) {
                existing = FindWindowW(window_class_name, nullptr);
                if (existing == nullptr) { Sleep(50); }
            }
            MessageBoxW(nullptr, L"French Accent Input이 이미 실행 중입니다.", app_name,
                        MB_OK | MB_ICONINFORMATION);
            if (existing != nullptr) {
                DWORD existing_process = 0;
                (void)GetWindowThreadProcessId(existing, &existing_process);
                if (existing_process != 0) { (void)AllowSetForegroundWindow(existing_process); }
                (void)PostMessageW(existing, open_usage_message, 0, 0);
            }
        }
        return 0;
    }

    app_instance = instance;
    caps_lock_on = (GetKeyState(VK_CAPITAL) & 1) != 0;
    caps_key_down = (GetAsyncKeyState(VK_CAPITAL) & 0x8000) != 0;
    taskbar_created_message = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = window_class_name;
    window_class.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_FAI_APP));

    if (RegisterClassExW(&window_class) == 0) {
        CloseHandle(mutex);
        return 1;
    }

    WNDCLASSEXW ui_class = window_class;
    ui_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    ui_class.lpfnWndProc = usage_proc;
    ui_class.lpszClassName = usage_class_name;
    if (RegisterClassExW(&ui_class) == 0) { CloseHandle(mutex); return 1; }
    ui_class.lpfnWndProc = settings_proc;
    ui_class.lpszClassName = settings_class_name;
    if (RegisterClassExW(&ui_class) == 0) { CloseHandle(mutex); return 1; }

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

    const bool virtual_test = allow_virtual_keyboard_input && command_line != nullptr &&
        (wcsstr(command_line, L"--virtual-e-cycle") != nullptr ||
         wcsstr(command_line, L"--virtual-e-cycle-delayed") != nullptr ||
         wcsstr(command_line, L"--virtual-unicode-e") != nullptr);
    if (virtual_test) {
        integration_direct_unicode = wcsstr(command_line, L"--virtual-unicode-e") != nullptr;
        if (wcsstr(command_line, L"--virtual-e-cycle-delayed") != nullptr) {
            Sleep(5000);
        }
        (void)PostMessageW(hidden_window, virtual_keyboard_message, 0, 0);
    } else if (action == StartupAction::normal && !allow_virtual_keyboard_input) {
        show_usage();
        show_launch_notification();
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
