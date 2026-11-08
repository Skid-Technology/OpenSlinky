// Minimal Win32 declarations for the readable reimplementations in src/engine/protected/.
// (The image calls these through the protector's encrypted import slots; here they are ordinary imports.)
#pragma once
#include <stddef.h>

typedef void *HANDLE, *HWND, *HMODULE, *HINSTANCE, *LPVOID;
typedef int BOOL;
typedef unsigned long DWORD;
typedef long long LONG_PTR, LRESULT;
typedef unsigned long long ULONG_PTR, WPARAM;
typedef long long LPARAM;
typedef unsigned int UINT;
typedef unsigned short WCHAR;
typedef long long LARGE_INTEGER_QUAD;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef DWORD (*THREAD_START)(LPVOID);

struct FILETIME { DWORD lo, hi; };
struct WIN32_FIND_DATAW {
    DWORD attributes;
    FILETIME created, accessed, written;
    DWORD size_hi, size_lo, reserved0, reserved1;
    wchar_t name[260];
    wchar_t alt_name[14];
};

#define WINAPI __stdcall
#define INVALID_HANDLE ((HANDLE)(LONG_PTR)-1)
#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 1u
#define CREATE_NEW 1u
#define CREATE_ALWAYS 2u
#define OPEN_EXISTING 3u
#define FILE_ATTRIBUTE_NORMAL 0x80u
#define FILE_ATTRIBUTE_DIRECTORY 0x10u
#define INVALID_FILE_SIZE 0xffffffffu
#define ERROR_FILE_NOT_FOUND 2u
#define ERROR_NO_MORE_FILES 0x12u
#define ERROR_FILE_EXISTS 0x50u
#define ERROR_ALREADY_EXISTS 0xb7u
#define GWLP_WNDPROC (-4)
#define SW_SHOWNORMAL 1

extern "C" {
__declspec(dllimport) BOOL WINAPI QueryPerformanceCounter(long long *count);
__declspec(dllimport) BOOL WINAPI QueryPerformanceFrequency(long long *freq);
__declspec(dllimport) DWORD WINAPI GetLastError();
__declspec(dllimport) BOOL WINAPI CreateDirectoryW(const wchar_t *path, void *security);
__declspec(dllimport) BOOL WINAPI DeleteFileW(const wchar_t *path);
__declspec(dllimport) HANDLE WINAPI CreateFileW(const wchar_t *path, DWORD access, DWORD share, void *security,
                                                DWORD disposition, DWORD flags, HANDLE templ);
__declspec(dllimport) DWORD WINAPI GetFileSize(HANDLE h, DWORD *high);
__declspec(dllimport) BOOL WINAPI ReadFile(HANDLE h, void *buf, DWORD n, DWORD *done, void *overlapped);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE h, const void *buf, DWORD n, DWORD *done, void *overlapped);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE h);
__declspec(dllimport) HANDLE WINAPI FindFirstFileW(const wchar_t *pattern, WIN32_FIND_DATAW *fd);
__declspec(dllimport) BOOL WINAPI FindNextFileW(HANDLE h, WIN32_FIND_DATAW *fd);
__declspec(dllimport) BOOL WINAPI FindClose(HANDLE h);
__declspec(dllimport) void WINAPI GetSystemTimeAsFileTime(FILETIME *ft);
__declspec(dllimport) HINSTANCE WINAPI ShellExecuteW(HWND hwnd, const wchar_t *op, const wchar_t *file,
                                                     const wchar_t *params, const wchar_t *dir, int show);
__declspec(dllimport) HMODULE WINAPI GetModuleHandleA(const char *name);
__declspec(dllimport) void *WINAPI GetProcAddress(HMODULE m, const char *name);
__declspec(dllimport) HWND WINAPI GetForegroundWindow();
__declspec(dllimport) BOOL WINAPI IsWindowUnicode(HWND hwnd);
__declspec(dllimport) DWORD WINAPI GetWindowThreadProcessId(HWND hwnd, DWORD *pid);
__declspec(dllimport) DWORD WINAPI GetCurrentProcessId();
__declspec(dllimport) HANDLE WINAPI CreateThread(void *security, size_t stack, THREAD_START start, LPVOID param,
                                                 DWORD flags, DWORD *id);
__declspec(dllimport) void WINAPI Sleep(DWORD ms);
__declspec(dllimport) void WINAPI FreeLibraryAndExitThread(HMODULE m, DWORD code);
__declspec(dllimport) int WINAPI WideCharToMultiByte(UINT codepage, DWORD flags, const wchar_t *src, int n, char *dst,
                                                 int dst_size, const char *def, BOOL *used_def);
__declspec(dllimport) int WINAPI MultiByteToWideChar(UINT codepage, DWORD flags, const char *src, int n, wchar_t *dst,
                                                 int dst_size);
__declspec(dllimport) HANDLE WINAPI GetStdHandle(DWORD which);
__declspec(dllimport) void WINAPI OutputDebugStringA(const char *text);
__declspec(dllimport) HMODULE WINAPI LoadLibraryA(const char *name);
__declspec(dllimport) BOOL WINAPI DisableThreadLibraryCalls(HMODULE m);
wchar_t *_wgetenv(const wchar_t *name);
}
