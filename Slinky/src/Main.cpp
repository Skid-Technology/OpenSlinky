#include <windows.h>
#include "ConsoleUI.hpp"
#include "Injector.hpp"
#include "ProcessUtil.hpp"
#include <cstdio>
#include <cwctype>
#include <string>
#include <vector>

using namespace Slinky;

// Titles used by the clients we support (lower-case compare).
static int TitleScore(const std::wstring& title) {
    std::wstring t = title;
    for (auto& ch : t) ch = (wchar_t)towlower(ch);
    int score = 0;
    if (t.find(L"minecraft") != std::wstring::npos) score += 4;
    if (t.find(L"lunar") != std::wstring::npos) score += 4;
    if (t.find(L"badlion") != std::wstring::npos) score += 4;
    if (t.find(L"1.8") != std::wstring::npos) score += 1;
    return score;
}

// Picks the window that really belongs to a Minecraft JVM. Any Java program with a window
// (launchers, IDEs, other tools) used to be treated as the game; now the JVM must have LWJGL
// loaded. After allowFallback is set we fall back to the best title match, then the first window.
static bool PickTarget(const std::vector<TargetWindow>& windows, bool allowFallback, TargetWindow& out) {
    int best = -1;
    for (const TargetWindow& w : windows) {
        if (!ProcessUtil::IsModuleLoaded(w.pid, L"lwjgl")) continue;
        int score = 10 + TitleScore(w.title);
        if (score > best) { best = score; out = w; }
    }
    if (best >= 0) return true;
    if (!allowFallback || windows.empty()) return false;
    best = -1;
    for (const TargetWindow& w : windows) {
        int score = TitleScore(w.title);
        if (score > best) { best = score; out = w; }
    }
    return true;
}

int main() {
    ConsoleUI::Initialize();
    ConsoleUI::StartAnimation();

    ConsoleUI::SetStatusText("Finding bundled DLL");
    std::wstring dllPath;
    std::wstring logPath;

    if (!Injector::ExtractBundledDll(dllPath, logPath)) {
        Sleep(2500);
        ConsoleUI::StopAnimation();
        ConsoleUI::Shutdown();
        return 1;
    }

    ConsoleUI::SetStatusText("SCANNING");

    DWORD startWait = GetTickCount();
    const DWORD timeoutWait = 180000;
    std::vector<TargetWindow> windows;
    TargetWindow target;
    bool found = false;
    int dotCount = 0;

    while (GetTickCount() - startWait < timeoutWait) {
        windows = ProcessUtil::FindMinecraftWindows();
        if (PickTarget(windows, GetTickCount() - startWait > 20000, target)) {
            found = true;
            break;
        }

        const char* dots[] = { "", ".", "..", "..." };
        char waitMsg[64];
        sprintf_s(waitMsg, "Waiting for Minecraft%s", dots[dotCount % 4]);
        ConsoleUI::SetStatusText(waitMsg);
        dotCount++;

        Sleep(500);
    }

    if (!found) {
        ConsoleUI::SetStatusText("Minecraft not found after 3 minutes.");
        Sleep(3000);
        ConsoleUI::StopAnimation();
        ConsoleUI::Shutdown();
        return 1;
    }

    char foundMsg[256];
    char titleUtf8[256] = { 0 };
    WideCharToMultiByte(CP_UTF8, 0, target.title.c_str(), -1, titleUtf8, 256, nullptr, nullptr);
    sprintf_s(foundMsg, "Found Minecraft %s", titleUtf8);
    ConsoleUI::SetStatusText(foundMsg);

    Sleep(500);

    ConsoleUI::SetStatusText("INJECTING ...");
    if (!Injector::InjectDll(target.pid, dllPath)) {
        Sleep(3000);
        ConsoleUI::StopAnimation();
        ConsoleUI::Shutdown();
        return 1;
    }

    bool success = Injector::MonitorStartup(target.pid, logPath);

    Sleep(3000);
    ConsoleUI::StopAnimation();
    ConsoleUI::Shutdown();

    return success ? 0 : 1;
}
