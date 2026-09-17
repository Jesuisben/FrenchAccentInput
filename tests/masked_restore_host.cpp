#include <windows.h>
#include <vector>
#include <cstdio>

HOOKPROC observed_keyboard_proc = nullptr;
LRESULT CALLBACK observe_keyboard(int code, WPARAM message, LPARAM parameter) {
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

// Diagnostic experiment: omit immediate Alt restoration after output. This is
// not a product candidate until native shortcuts can restore Alt on demand.
UINT WINAPI masked_restore_send_input(UINT count, LPINPUT inputs, int size) {
    std::vector<INPUT> experiment(inputs, inputs + count);
    if (!experiment.empty() && experiment.back().ki.dwExtraInfo == 0x46414931 &&
        experiment.back().ki.wVk == VK_LMENU && experiment.back().ki.dwFlags == 0) {
        experiment.pop_back();
    }
    if (experiment.empty()) { return count; }
    const UINT sent = SendInput(static_cast<UINT>(experiment.size()), experiment.data(), size);
    return sent == experiment.size() ? count : 0;
}

#define SendInput masked_restore_send_input
#define SetWindowsHookExW observed_hook
#include "../src/main.cpp"
#undef SetWindowsHookExW
#undef SendInput
