// Readable reimplementations of protector-obfuscated engine functions (0x4bcb0 - 0x4c390):
// Win32 lookups, the game-window input hook and the self-unload thread.
//
// The client subclasses the game window (hook_game_window). Its window procedure (game_window_proc) turns window messages
// into input events and hands them to the client's input handler (input_input_handler_hooked_into_game); a handler returning true
// swallows the message, otherwise it goes on to the game's original window procedure. Synthetic clicks
// (auto clicker) are posted to the window as private messages 0x8cf3..0x8cf6 (post_synthetic_click) and become real
// WM_*BUTTON* messages after the handler has seen them.
//
// Marked "inferred": the image hides that API behind an encrypted import slot and it was named from its
// arguments and how the result is used.
#include "slinky/engine.h"
#include "slinky/win32.h"
#include "slinky/platform.h"

FN(int, windows_key_to_client_key, (unsigned long long vk));   // Windows virtual key -> client key code (0 = unmapped)

// ---- kernel32 lookups ----
extern "C" HMODULE kernel32_lookups(const char *module) { return GetModuleHandleA(module); }
extern "C" void *get_proc_address(HMODULE m, const char *name) { return GetProcAddress(m, name); }

// ---- input event handed to the client's handler ----
enum InputType { IN_MOVE = 1, IN_KEY = 2, IN_WHEEL = 3, IN_RAW = 4 };
struct InputEvent {
    int type;
    union {
        struct { int x, y; } move;                 // IN_MOVE: client-area position
        struct {
            int code;                              // IN_KEY: client key code / mouse button (1 L, 2 R, 3 M, 4/5 X1/X2)
            char text[4];                          //   character input: UTF-8 (code 0)
            unsigned char down;
            unsigned char synthetic;               //   posted by the auto clicker
            char pad[2];
        } key;
        struct { float delta; } wheel;             // IN_WHEEL: notches (+ away from the user)
    };
};
typedef bool (*InputHandler)(InputEvent *e);
typedef LONG_PTR(WINAPI *SetWindowLongPtrF)(HWND, int, LONG_PTR);
typedef LRESULT(WINAPI *CallWindowProcF)(WNDPROC, HWND, UINT, WPARAM, LPARAM);
typedef UINT(WINAPI *MapVirtualKeyF)(UINT, UINT);
typedef BOOL(WINAPI *PostMessageF)(HWND, UINT, WPARAM, LPARAM);

extern "C" InputHandler input_input_handler_hooked_into_game;          // client input handler (installed elsewhere)
extern "C" WNDPROC input_game_s_original_window_procedure;               // the game's original window procedure (0 = not hooked)
extern "C" SetWindowLongPtrF input_w_variant_chosen_by_window;     // A or W variant, chosen by the window's charset
static HWND input_hooked_window;                                      // window that was actually subclassed
extern "C" MapVirtualKeyF pfn_MapVirtualKey;
extern "C" CallWindowProcF pfn_CallWindowProc;
extern "C" PostMessageF pfn_PostMessage;
// user32 function by name, ANSI or Unicode variant (the image reads them from its import slots)
static void *user32(const char *base, bool wide) {
    char name[32];
    int n = 0;
    while (base[n]) { name[n] = base[n]; n++; }
    name[n++] = wide ? 'W' : 'A';
    name[n] = 0;
    return GetProcAddress(GetModuleHandleA("user32"), name);
}

// private messages posted by post_synthetic_click
enum { MSG_LEFT_DOWN = 0x8cf3, MSG_LEFT_UP = 0x8cf4, MSG_RIGHT_DOWN = 0x8cf5, MSG_RIGHT_UP = 0x8cf6 };

static LRESULT pass_on(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) { return pfn_CallWindowProc(input_game_s_original_window_procedure, hwnd, msg, wp, lp); }
static bool send_key(int code, bool down) {
    InputEvent e = {};
    e.type = IN_KEY;
    e.key.code = code;
    e.key.down = down;
    return input_input_handler_hooked_into_game(&e);
}

// ---- the hooked window procedure ----
extern "C" LRESULT game_window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    InputEvent e = {};
    switch (msg) {
    case 0x0002: {  // WM_DESTROY: the original procedure is gone with the window
        WNDPROC original = input_game_s_original_window_procedure;
        input_game_s_original_window_procedure = 0;
        input_hooked_window = 0;
        if (original && pfn_CallWindowProc) return pfn_CallWindowProc(original, hwnd, msg, wp, lp);
        return 0;
    }
    case 0x00ff:   // WM_INPUT (raw input arrived)
        e.type = IN_RAW;
        if (input_input_handler_hooked_into_game(&e)) return 0;
        break;
    case 0x0100: case 0x0101: case 0x0104: case 0x0105: {   // WM_(SYS)KEYDOWN / WM_(SYS)KEYUP
        unsigned long long vk = wp;
        bool extended = lp & 0x1000000;
        if (wp == 0x10) vk = pfn_MapVirtualKey((lp >> 16) & 0xff, 3);   // shift: VSC_TO_VK_EX gives L/R shift
        else if (wp == 0x11) vk = extended ? 0xa3 : 0xa2;         // right / left control
        else if (wp == 0x12) vk = extended ? 0xa5 : 0xa4;         // right / left alt
        int code = windows_key_to_client_key(vk);
        if (code && send_key(code, (msg & ~4u) == 0x100)) return 0;
        break;
    }
    case 0x0102: {   // WM_CHAR: UTF-16 unit -> UTF-8
        wchar_t wc = (wchar_t)wp;
        e.type = IN_KEY;
        e.key.down = 1;
        if (WideCharToMultiByte(65001, 0x200, &wc, 1, e.key.text, 4, 0, 0) && input_input_handler_hooked_into_game(&e)) return 0;
        break;
    }
    case 0x0200:   // WM_MOUSEMOVE
        e.type = IN_MOVE;
        e.move.x = (short)(lp & 0xffff);
        e.move.y = (short)(lp >> 16);
        // when the handler swallows a move the image also runs one protected call on data_304bc8
        // (not recovered: its import slot is only filled at load time)
        if (input_input_handler_hooked_into_game(&e)) return 0;
        break;
    case 0x0201: case 0x0202:   // left button
        if (send_key(1, msg == 0x0201)) return 0;
        break;
    case 0x0204: case 0x0205:   // right button
        if (send_key(2, msg == 0x0204)) return 0;
        break;
    case 0x0207: case 0x0208:   // middle button
        if (send_key(3, msg == 0x0207)) return 0;
        break;
    case 0x020a:   // WM_MOUSEWHEEL
        e.type = IN_WHEEL;
        e.wheel.delta = (float)(short)(wp >> 16) / 120.0f;
        if (input_input_handler_hooked_into_game(&e)) return 0;
        break;
    case 0x020b: case 0x020c:   // X buttons: XBUTTON1 = 4, XBUTTON2 = 5
        if (send_key((wp & 0xffff0000) == 0x10000 ? 4 : 5, msg == 0x020b)) return 0;
        break;
    case MSG_LEFT_DOWN: case MSG_LEFT_UP:
    case MSG_RIGHT_DOWN: case MSG_RIGHT_UP: {   // synthetic click from the auto clicker
        bool left = msg <= MSG_LEFT_UP, down = msg == MSG_LEFT_DOWN || msg == MSG_RIGHT_DOWN;
        e.type = IN_KEY;
        e.key.code = left ? 1 : 2;
        e.key.down = down;
        e.key.synthetic = 1;
        if (input_input_handler_hooked_into_game(&e)) return 0;
        msg = left ? (down ? 0x201 : 0x202) : (down ? 0x204 : 0x205);   // deliver as a real click
        break;
    }
    }
    return pass_on(hwnd, msg, wp, lp);
}

// ---- hook the game window once an input handler exists (called every frame) ----
extern "C" void hook_game_window() {
    if (!input_input_handler_hooked_into_game || input_game_s_original_window_procedure) return;
    HWND hwnd = input_hooked_window;
    if (!hwnd) {
        hwnd = GetForegroundWindow();   // inferred: argument-less user32 call returning the game window
        if (!hwnd) return;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (!pid || pid != GetCurrentProcessId()) return;   // never subclass another process's window
        input_hooked_window = hwnd;
    }
    bool w = IsWindowUnicode(hwnd);   // use the variants that match how the game created its window
    pfn_MapVirtualKey = (MapVirtualKeyF)user32("MapVirtualKey", w);
    pfn_CallWindowProc = (CallWindowProcF)user32("CallWindowProc", w);
    input_w_variant_chosen_by_window = (SetWindowLongPtrF)user32("SetWindowLongPtr", w);
    pfn_PostMessage = (PostMessageF)user32("PostMessage", w);
    if (!input_w_variant_chosen_by_window || !pfn_CallWindowProc || !pfn_MapVirtualKey || !pfn_PostMessage) {
        input_hooked_window = 0;
        return;
    }
    LONG_PTR previous = input_w_variant_chosen_by_window(hwnd, GWLP_WNDPROC, (LONG_PTR)game_window_proc);
    if (!previous) {
        input_hooked_window = 0;
        return;
    }
    input_game_s_original_window_procedure = (WNDPROC)previous;
}

// ---- undo hook_game_window (called once while unloading) ----
// inferred: the body is virtualised; its place in the unload sequence (after the hooks are disabled, before
// the unload thread) and the state hook_game_window leaves behind make this the counterpart of hook_game_window.
// The HWND is remembered from the install: restoring on a different foreground window would leave the
// actually-subclassed window pointing at game_window_proc after this DLL is freed.
extern "C" void restore_game_window() {
    if (!input_game_s_original_window_procedure) return;
    HWND hwnd = input_hooked_window;
    if (hwnd && input_w_variant_chosen_by_window)
        input_w_variant_chosen_by_window(hwnd, GWLP_WNDPROC, (LONG_PTR)input_game_s_original_window_procedure);
    input_game_s_original_window_procedure = 0;
    input_hooked_window = 0;
}

// ---- synthetic mouse click: left (or right) button down (or up), delivered through game_window_proc ----
extern "C" void post_synthetic_click(bool left, bool down) {
    if (!pfn_PostMessage) return;
    UINT msg = 0x8cf4 + (left ? 0 : 2) - (down ? 1 : 0);   // 0x8cf3 L down, 0x8cf4 L up, 0x8cf5 R down, 0x8cf6 R up
    pfn_PostMessage(input_hooked_window ? input_hooked_window : GetForegroundWindow(), msg, 0, 0);
}

// ---- unload: a thread waits a second (so the hooks have returned) then frees the DLL ----
extern "C" DWORD unload_thread(LPVOID module) {
    slinky_logf("unload thread: waiting for the hooks to be out");
    Sleep(1000);
    slinky_logf("unload thread: teardown");
    if (!slinky_teardown()) {   // release what start-up built (init.cpp), then the DLL itself
        slinky_logf("unload thread: teardown aborted, hooks are still live; keeping the client loaded");
        return 0;
    }
    slinky_logf("unload thread: FreeLibrary");
    FreeLibraryAndExitThread((HMODULE)module, 0);
    return 0;
}
extern "C" void start_unload_thread(HMODULE self) { CloseHandle(CreateThread(0, 0, unload_thread, self, 0, 0)); }
