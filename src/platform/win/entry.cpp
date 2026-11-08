// Windows entry of the client DLL: it is injected into the game process (LoadLibrary), so DllMain only starts the
// start-up thread (slinky_setup runs JNI and file I/O, which must not happen under the loader lock).
#include "slinky/platform.h"
#include "slinky/win32.h"

extern "C" void start_unload_thread(HMODULE self);   // protected/input.cpp: unload thread (teardown, then FreeLibraryAndExitThread)

static DWORD start_thread(LPVOID self) {
    if (slinky_setup(self)) start_unload_thread((HMODULE)self);   // failed: report was printed, take the DLL out again
    return 0;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE self, DWORD reason, void *) {
    if (reason == 1 /* DLL_PROCESS_ATTACH */) {
        DisableThreadLibraryCalls(self);
        CloseHandle(CreateThread(0, 0, start_thread, self, 0, 0));
    }
    return 1;
}

// ---- diagnostics: debugger output, the process's stderr handle and %TEMP%\slinky.log ----
// (launchers do not keep the game's stderr, and an unload problem must stay visible after the exit)
extern "C" {
__declspec(dllimport) DWORD WINAPI GetTempPathW(DWORD n, wchar_t *buf);
}
extern "C" void slinky_log_write(const char *line) {
    char buf[600];
    int n = 0;
    for (const char *p = "Slinky: "; *p; p++) buf[n++] = *p;
    for (; *line && n < 597; line++) buf[n++] = *line;
    buf[n++] = '\r';
    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);
    DWORD done;
    HANDLE err = GetStdHandle((DWORD)-12 /* STD_ERROR_HANDLE */);
    if (err && err != INVALID_HANDLE) WriteFile(err, buf, (DWORD)n, &done, 0);
    wchar_t path[320];
    DWORD len = GetTempPathW(300, path);
    if (len && len < 300) {
        const wchar_t name[] = L"slinky.log";
        for (int i = 0; i <= 10; i++) path[len + i] = name[i];
        HANDLE f = CreateFileW(path, 4 /* FILE_APPEND_DATA */, FILE_SHARE_READ | 2 /* FILE_SHARE_WRITE */,
                              0, 4 /* OPEN_ALWAYS */, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE) {
            WriteFile(f, buf, (DWORD)n, &done, 0);
            CloseHandle(f);
        }
    }
}

// ---- OpenGL: wglGetProcAddress for extensions, opengl32 itself for GL 1.1 ----
typedef void *(WINAPI *WglGetProcAddress)(const char *);
extern "C" void *slinky_gl_get_proc(const char *name) {
    static HMODULE gl;
    static WglGetProcAddress wgl;
    if (!gl) {
        gl = GetModuleHandleA("opengl32");
        if (!gl) gl = LoadLibraryA("opengl32.dll");
        if (!gl) return 0;
        wgl = (WglGetProcAddress)GetProcAddress(gl, "wglGetProcAddress");
    }
    void *p = wgl ? wgl(name) : 0;
    if (p && p != (void *)1 && p != (void *)2 && p != (void *)3 && p != (void *)-1) return p;   // wgl failure values
    return GetProcAddress(gl, name);
}
