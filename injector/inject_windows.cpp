// Windows injection: CreateRemoteThread(LoadLibraryW) into the game's process.
// The client DLL's DllMain starts the client, so loading it is the whole job.
#ifdef _WIN32

#include "inject.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include <windows.h>
#include <tlhelp32.h>

namespace Injector {
namespace {

std::string narrow(const std::wstring &text) {
    if (text.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string out((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring widen(const std::string &text) {
    if (text.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), out.data(), n);
    return out;
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return text;
}

std::string process_name(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return {};
    wchar_t path[1024];
    DWORD size = (DWORD)(sizeof path / sizeof path[0]);
    std::string result;
    if (QueryFullProcessImageNameW(process, 0, path, &size)) {
        std::wstring full(path, size);
        size_t slash = full.find_last_of(L"\\/");
        result = narrow(slash == std::wstring::npos ? full : full.substr(slash + 1));
    }
    CloseHandle(process);
    return result;
}

}  // namespace

std::vector<Process> ListProcesses() {
    std::vector<Process> out;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return out;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            out.push_back(Process{entry.th32ProcessID, narrow(entry.szExeFile), {}});
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    std::sort(out.begin(), out.end(), [](const Process &a, const Process &b) {
        if (a.name != b.name) return a.name < b.name;
        return a.pid < b.pid;
    });
    return out;
}

bool FindMinecraft(Process &out, std::string &error) {
    const wchar_t *classes[] = {L"LWJGL", L"GLFW30"};
    for (const wchar_t *cls : classes) {
        HWND window = FindWindowW(cls, nullptr);
        if (!window) continue;
        DWORD pid = 0;
        GetWindowThreadProcessId(window, &pid);
        if (!pid) continue;
        out.pid = pid;
        out.name = process_name(pid);
        return true;
    }

    // No window class match: fall back to the first javaw.exe/java.exe, which
    // is right when the game is still starting or the window class differs.
    for (const Process &process : ListProcesses()) {
        std::string name = lower(process.name);
        if (name == "javaw.exe" || name == "java.exe") {
            out = process;
            return true;
        }
    }

    error = "Minecraft is not running (no LWJGL/GLFW30 window and no javaw.exe)";
    return false;
}

bool FindTarget(const std::string &target, Process &out, std::string &error) {
    if (target.empty()) {
        error = "no target given";
        return false;
    }

    bool numeric = std::all_of(target.begin(), target.end(), [](unsigned char c) { return std::isdigit(c); });
    if (numeric) {
        out.pid = std::strtoul(target.c_str(), nullptr, 10);
        out.name = process_name((DWORD)out.pid);
        if (out.pid && !out.name.empty()) return true;
        error = "no process with pid " + target;
        return false;
    }

    std::string needle = lower(target);
    auto all = ListProcesses();
    for (const Process &process : all) {
        if (lower(process.name) == needle) {
            out = process;
            return true;
        }
    }
    for (const Process &process : all) {
        if (lower(process.name).find(needle) != std::string::npos) {
            out = process;
            return true;
        }
    }
    error = "no process matching '" + target + "'";
    return false;
}

bool Inject(unsigned long pid, const std::string &library, const LogFn &log) {
    std::wstring wide_library = widen(library);

    DWORD full_size = GetFullPathNameW(wide_library.c_str(), 0, nullptr, nullptr);
    if (full_size) {
        std::wstring full(full_size, L'\0');
        DWORD written = GetFullPathNameW(wide_library.c_str(), full_size, full.data(), nullptr);
        if (written && written < full_size) {
            full.resize(written);
            wide_library = full;
        }
    }

    log("injecting " + narrow(wide_library) + " into pid " + std::to_string(pid));

    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                     PROCESS_VM_WRITE | PROCESS_VM_READ,
                                 FALSE, (DWORD)pid);
    if (!process) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) log("access denied -- try running the injector as administrator");
        else log("OpenProcess failed (" + std::to_string(err) + ")");
        return false;
    }

    SIZE_T bytes = (wide_library.size() + 1) * sizeof(wchar_t);
    void *remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        log("VirtualAllocEx failed (" + std::to_string(GetLastError()) + ")");
        CloseHandle(process);
        return false;
    }

    bool ok = false;
    SIZE_T written = 0;
    if (!WriteProcessMemory(process, remote, wide_library.c_str(), bytes, &written)) {
        log("WriteProcessMemory failed (" + std::to_string(GetLastError()) + ")");
    } else {
        HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
        auto load_library = reinterpret_cast<LPTHREAD_START_ROUTINE>(
            GetProcAddress(kernel, "LoadLibraryW"));
        if (!load_library) {
            log("could not find LoadLibraryW");
        } else {
            HANDLE thread = CreateRemoteThread(process, nullptr, 0, load_library, remote, 0, nullptr);
            if (!thread) {
                log("CreateRemoteThread failed (" + std::to_string(GetLastError()) + ")");
            } else {
                WaitForSingleObject(thread, INFINITE);
                DWORD exit_code = 0;
                if (GetExitCodeThread(thread, &exit_code)) {
                    // The HMODULE return is truncated to a DWORD here; only
                    // zero (failure) is meaningful.
                    ok = exit_code != 0;
                    if (!ok) log("LoadLibraryW returned NULL in the target");
                } else {
                    log("GetExitCodeThread failed (" + std::to_string(GetLastError()) + ")");
                }
                CloseHandle(thread);
            }
        }
    }

    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);
    return ok;
}

std::string EnvironmentHint() {
    return "if injection fails with access denied, run the injector as administrator";
}

}  // namespace Injector

#endif  // _WIN32
