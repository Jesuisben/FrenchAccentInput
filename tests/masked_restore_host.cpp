#include <windows.h>
#include <vector>
#include <cstdio>

HOOKPROC observed_keyboard_proc = nullptr;
unsigned output_depth = 0;
LRESULT CALLBACK observe_keyboard(int code, WPARAM message, LPARAM parameter) {
    if (code >= 0 && output_depth != 0 &&
        reinterpret_cast<const KBDLLHOOKSTRUCT*>(parameter)->dwExtraInfo == 0x46414932) {
        std::fprintf(stderr, "REENTRANT driver event during output depth=%u\n", output_depth);
    }
    const LRESULT result = observed_keyboard_proc(code, message, parameter);
    if (code >= 0) {
        const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(parameter);
        // Only this project's fixed synthetic driver/output; never physical or
        // unrelated injected input. This diagnostic is not in the product.
        if (event->dwExtraInfo == 0x46414932 || event->dwExtraInfo == 0x46414931) {
            std::fprintf(stderr, "test event marker=%llu vk=%lu msg=%llu suppress=%lld\n",
                         static_cast<unsigned long long>(event->dwExtraInfo), event->vkCode,
                         static_cast<unsigned long long>(message), static_cast<long long>(result));
            std::fflush(stderr);
        }
    }
    return result;
}
HHOOK WINAPI observed_hook(int type, HOOKPROC proc, HINSTANCE instance, DWORD thread) {
    if (type == WH_KEYBOARD_LL) {
        observed_keyboard_proc = proc;
        return SetWindowsHookExW(type, observe_keyboard, instance, thread);
    }
    return SetWindowsHookExW(type, proc, instance, thread);
}

// Diagnostic experiment: use synchronous native RichEdit editing, retaining
// production's modifier sequence. Other targets keep the original SendInput.
UINT WINAPI masked_restore_send_input(UINT count, LPINPUT inputs, int size) {
    GUITHREADINFO info{};
    info.cbSize = sizeof(info);
    wchar_t name[128]{};
    const HWND foreground = GetForegroundWindow();
    if ((count != 6 && count != 8) ||
        !GetGUIThreadInfo(GetWindowThreadProcessId(foreground, nullptr), &info) ||
        GetClassNameW(info.hwndFocus, name, 128) == 0 ||
        wcsncmp(name, L"RichEdit", 8) != 0) {
        return SendInput(count, inputs, size);
    }
    if (SendInput(3, inputs, size) != 3) { return 0; }
    DWORD_PTR selection = 0;
    if (!SendMessageTimeoutW(info.hwndFocus, EM_GETSEL, 0, 0,
                             SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &selection)) { return 0; }
    const WORD start = LOWORD(selection);
    const WORD end = HIWORD(selection);
    if (count == 8 && start == end && start != 0) {
        DWORD_PTR ignored = 0;
        if (!SendMessageTimeoutW(info.hwndFocus, EM_SETSEL, start - 1, end,
                                 SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &ignored)) { return 0; }
    }
    const wchar_t text[] = {static_cast<wchar_t>(inputs[count - 3].ki.wScan), L'\0'};
    DWORD_PTR ignored = 0;
    const bool edited = SendMessageTimeoutW(info.hwndFocus, EM_REPLACESEL, TRUE,
                         reinterpret_cast<LPARAM>(text), SMTO_ABORTIFHUNG | SMTO_BLOCK,
                         100, &ignored) != 0;
    const UINT restored = SendInput(1, &inputs[count - 1], size);
    return edited && restored == 1 ? count : 0;
}

#define SendInput masked_restore_send_input
#define SetWindowsHookExW observed_hook
#include "../src/main.cpp"
#undef SetWindowsHookExW
#undef SendInput
