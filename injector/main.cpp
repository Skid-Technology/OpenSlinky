// slinky-inject: the loader's terminal UI and a cross-platform injector.
//
//   slinky-inject                          find Minecraft, inject libslinky.so /
//                                          slinky.dll from beside the injector
//   slinky-inject <pid|name>               same, but choose the target
//   slinky-inject <pid|name> <library>     inject that library
//   slinky-inject --lib <library>          choose the library only
//   slinky-inject --pid <pid>              choose the target only
//   slinky-inject --list                   list visible processes
//
// Linux uses ptrace + the target's own dlopen; Windows uses
// CreateRemoteThread(LoadLibraryW).  The animation is the one from the Go
// loader (animation/animation.go).
#include "inject.hpp"
#include "tui.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace {

struct Options {
    bool list = false;
    bool help = false;
    bool no_anim = false;
    bool pid_given = false;
    unsigned long pid = 0;
    std::string target;
    std::string library;
};

void usage(const char *program) {
    std::cout <<
        "usage: " << program << " [options] [target] [library]\n"
        "\n"
        "  target    pid or process name (default: the running Minecraft)\n"
        "  library   path to libslinky.so / slinky.dll (default: beside this program)\n"
        "\n"
        "options:\n"
        "  -p, --pid <pid>      target a pid\n"
        "  -n, --name <name>    target a process name (substring match)\n"
        "  -l, --lib <path>     library to inject\n"
        "      --list           list processes and exit\n"
        "      --no-anim        plain output, no animation\n"
        "  -h, --help           this text\n";
}

bool parse_options(int argc, char **argv, Options &options, std::string &error) {
    std::vector<std::string> positional;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        auto value = [&](const char *what) -> bool {
            if (i + 1 >= argc) {
                error = std::string(what) + " needs a value";
                return false;
            }
            return true;
        };

        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "--list") {
            options.list = true;
        } else if (arg == "--no-anim") {
            options.no_anim = true;
        } else if (arg == "-p" || arg == "--pid") {
            if (!value("--pid")) return false;
            char *end = nullptr;
            options.pid = std::strtoul(argv[++i], &end, 10);
            options.pid_given = true;
            if (!end || *end || !options.pid) {
                error = "invalid pid '" + std::string(argv[i]) + "'";
                return false;
            }
        } else if (arg == "-n" || arg == "--name") {
            if (!value("--name")) return false;
            options.target = argv[++i];
        } else if (arg == "-l" || arg == "--lib") {
            if (!value("--lib")) return false;
            options.library = argv[++i];
        } else if (!arg.empty() && arg[0] == '-') {
            error = "unknown option '" + arg + "'";
            return false;
        } else {
            positional.push_back(arg);
        }
    }

    // One positional that names a library is a library, not a target.
    if (positional.size() == 1 && options.target.empty() && !options.pid_given) {
        std::string lower = positional[0];
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (lower.size() > 3 && (lower.substr(lower.size() - 3) == ".so" ||
                                 lower.substr(lower.size() - 4) == ".dll")) {
            options.library = positional[0];
            positional.clear();
        }
    }

    if (positional.size() > 2) {
        error = "too many positional arguments";
        return false;
    }
    if (positional.size() >= 1 && options.target.empty() && !options.pid_given) options.target = positional[0];
    if (positional.size() >= 2 && options.library.empty()) options.library = positional[1];
    return true;
}

std::filesystem::path executable_path() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(std::wstring(buffer, n));
#else
    char buffer[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buffer, sizeof buffer - 1);
    if (n <= 0) return {};
    buffer[n] = 0;
    return std::filesystem::path(buffer);
#endif
}

std::string default_library() {
#ifdef _WIN32
    const char *names[] = {"slinky.dll", "libslinky.dll"};
    const char *build[] = {"build/cmake/slinky.dll", "../build/cmake/slinky.dll"};
#else
    const char *names[] = {"libslinky.so", "slinky.so"};
    const char *build[] = {"build/linux/libslinky.so", "../build/linux/libslinky.so"};
#endif

    std::error_code ec;
    std::filesystem::path exe = executable_path();
    std::vector<std::filesystem::path> candidates;

    if (!exe.empty()) {
        for (const char *name : names) candidates.push_back(exe.parent_path() / name);
    }
    for (const char *name : names) candidates.push_back(std::filesystem::current_path(ec) / name);
    for (const char *path : build) candidates.push_back(std::filesystem::current_path(ec) / path);
#ifndef _WIN32
    candidates.push_back("/usr/local/lib/libslinky.so");
    candidates.push_back("/usr/lib/libslinky.so");
#endif

    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate, ec)) {
            auto canonical = std::filesystem::weakly_canonical(candidate, ec);
            return ec ? candidate.string() : canonical.string();
        }
    }
    return {};
}

void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void render_for(Tui::Screen &screen, int milliseconds, const std::string &title, const std::string &message,
                bool error = false) {
    auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    do {
        screen.Render(title, message, error);
        sleep_ms(16);
    } while (std::chrono::steady_clock::now() < end);
}

void print_list(const std::vector<Injector::Process> &processes) {
    std::cout << "   PID  NAME                 COMMAND\n";
    for (const auto &process : processes) {
        std::string command = process.command_line;
        if (command.size() > 100) command = command.substr(0, 97) + "...";
        std::cout << std::setw(6) << process.pid << "  " << std::setw(20) << std::left << process.name << " "
                  << command << "\n";
    }
}

}  // namespace

int main(int argc, char **argv) {
    Options options;
    std::string error;
    if (!parse_options(argc, argv, options, error)) {
        std::cerr << "slinky-inject: " << error << "\n\n";
        usage(argv[0]);
        return 1;
    }
    if (options.help) {
        usage(argv[0]);
        return 0;
    }

    if (options.list) {
        print_list(Injector::ListProcesses());
        return 0;
    }

    const bool interactive = Tui::EnableAnsi() && Tui::StdoutIsTty() && !options.no_anim;
    Tui::Screen screen(interactive);

    if (!options.library.empty()) {
        std::error_code ec;
        if (!std::filesystem::exists(options.library, ec)) {
            std::cerr << "slinky-inject: library not found: " << options.library << "\n";
            return 1;
        }
    } else {
        options.library = default_library();
        if (options.library.empty()) {
            if (interactive) {
                screen.Render("ERROR",
                              "Library not found. Put libslinky.so/slinky.dll beside the injector, "
                              "or pass its path as the second argument.",
                              true);
                sleep_ms(3000);
            } else {
                std::cerr << "slinky-inject: library not found (pass its path as the second argument)\n";
            }
            return 1;
        }
    }

    // ---- find the target ----
    Injector::Process target;
    if (interactive) {
        render_for(screen, 450, "FINDING MINECRAFT", "Looking for the game process...");
    } else {
        std::cout << "Finding Minecraft...\n";
    }

    bool found = false;
    if (options.pid_given) {
        target.pid = options.pid;
        found = true;
    } else if (!options.target.empty()) {
        found = Injector::FindTarget(options.target, target, error);
    } else {
        found = Injector::FindMinecraft(target, error);
    }

    if (!found) {
        std::string hint = Injector::EnvironmentHint();
        std::string message = error;
        if (!hint.empty()) message += "\n\n" + hint;
        if (interactive) {
            screen.Render("ERROR", message, true);
            sleep_ms(4000);
        } else {
            std::cerr << "slinky-inject: " << message << "\n";
        }
        return 1;
    }

    // ---- inject, with the animation still running ----
    std::string last_log = "Injecting into Minecraft...";
    std::mutex log_mutex;
    std::atomic<bool> done{false};
    std::atomic<bool> injected{false};

    auto started = std::chrono::steady_clock::now();
    std::thread worker([&]() {
        injected.store(Injector::Inject(
            target.pid, options.library,
            [&](const std::string &line) {
                std::lock_guard<std::mutex> lock(log_mutex);
                last_log = line;
            }));
        done.store(true);
    });

    if (interactive) {
        for (;;) {
            {
                std::lock_guard<std::mutex> lock(log_mutex);
                screen.Render("INJECTING", last_log);
            }
            sleep_ms(16);
            bool finished = done.load();
            auto elapsed = std::chrono::steady_clock::now() - started;
            if (finished && elapsed > std::chrono::milliseconds(1200)) break;
        }
    }
    worker.join();

    if (!injected.load()) {
        std::string hint = Injector::EnvironmentHint();
        std::string message = "Injection failed. " + last_log;
        if (!hint.empty()) message += "\n\n" + hint;
        if (interactive) {
            screen.Render("ERROR", message, true);
            sleep_ms(5000);
        } else {
            std::cerr << "slinky-inject: " << message << "\n";
        }
        return 1;
    }

    // ---- success countdown, exactly the Go loader's five seconds ----
    if (interactive) {
        for (int i = 5; i > 0; --i) {
            std::string title = "Injected successfully. Loader will close in " + std::to_string(i) + "s.";
            render_for(screen, 1000, title, "DONE");
        }
    } else {
        std::cout << "Injected successfully into pid " << target.pid << ".\n";
    }
    return 0;
}
