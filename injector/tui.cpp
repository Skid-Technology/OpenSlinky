#include "tui.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <thread>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Tui {
namespace {

// Exact frames from the Go loader.  The trailing spaces are part of the art:
// they keep the right edge of the box aligned between frames.
const char *kLogo[4] = {
    " _                            \n"
    "((                  ____      \n"
    " \\\\________________( ( e\\____o\n"
    " (                     _____/ \n"
    "  \\___________________/       \n"
    "  /   \\           \\  /        ",

    "  _                           \n"
    " ((                 ____      \n"
    " ||________________( ( e\\____o\n"
    " (                     _____/ \n"
    "  \\___________________/       \n"
    "   )  |           |   )       ",

    "   _                          \n"
    "  //                ____      \n"
    " //________________( ( e\\____o\n"
    " (                     _____/ \n"
    "  \\___________________/       \n"
    "   \\  /           /   \\       ",

    "  _                           \n"
    " ((                 ____      \n"
    " ||________________( ( e\\____o\n"
    " (                     _____/ \n"
    "  \\___________________/       \n"
    "  |   )           )   |       ",
};

const char *kErrorLogo =
    "\n"
    "                    ____      \n"
    "  _________________( ( x\\____o\n"
    "//                     _____/ \n"
    "||____________________/       \n"
    "  |   |           |  |        ";

const char *kOrange = "\x1b[38;5;208m";
const char *kRed    = "\x1b[91m";
const char *kReset  = "\x1b[0m";

std::string make_line() {
    std::random_device device;
    std::mt19937 rng(device());
    std::string line;
    line.reserve(25);
    for (int i = 0; i < 25; i++) line.push_back(rng() & 1 ? '_' : '-');
    return line;
}

const std::string &base_line() {
    static const std::string line = make_line();
    return line;
}

}  // namespace

bool EnableAnsi() {
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!out || out == INVALID_HANDLE_VALUE || !GetConsoleMode(out, &mode)) return false;
    return SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return true;
#endif
}

bool StdoutIsTty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

std::string Logo(int frame) {
    return kLogo[((frame % 4) + 4) % 4];
}

std::string ErrorLogo() { return kErrorLogo; }

// The line is the base pattern shifted one character per frame, exactly as the
// Go loader's init() builds its 30 frames.
std::string SlinkyLine(int frame) {
    const std::string &base = base_line();
    if (base.empty()) return {};
    int shift = ((frame % (int)base.size()) + (int)base.size()) % (int)base.size();
    return base.substr(shift) + base.substr(0, shift);
}

Screen::Screen(bool animate) : animate_(animate && StdoutIsTty()) {
    if (animate_) {
        std::fputs("\x1b[?1049h\x1b[?25l", stdout);
        std::fflush(stdout);
    }
}

Screen::~Screen() {
    if (animate_) {
        std::fputs("\x1b[?25h\x1b[?1049l", stdout);
        std::fflush(stdout);
    }
}

void Screen::Render(const std::string &title, const std::string &message, bool error) {
    if (!animate_) {
        // Non-interactive output: one line per state change is enough.
        std::fputs(error ? "ERROR: " : "", stdout);
        std::fputs(title.c_str(), stdout);
        if (!message.empty()) {
            std::fputs(" - ", stdout);
            std::fputs(message.c_str(), stdout);
        }
        std::fputc('\n', stdout);
        std::fflush(stdout);
        return;
    }

    // Build the whole frame first.  Clearing the screen before every 16 ms redraw made the
    // console blank between frames (the flicker); instead the cursor goes home and each line is
    // overwritten in one write, with erase-to-end-of-line cleaning up shorter content.
    const int frame_index = Frame();

    std::string body;
    body.reserve(1024);
    if (error) {
        body += kRed;
        body += kErrorLogo;
    } else {
        body += kLogo[frame_index % 4];
    }
    body += '\n';
    body += SlinkyLine(frame_index);
    body += "\n\n> ";
    body += (error ? kRed : kOrange);
    body += title;
    body += kReset;
    body += "\n\n";
    body += message;
    body += '\n';

    // Nothing changed since the last call: leave the terminal alone.
    if (body == last_body_) return;
    last_body_ = body;

    std::string out;
    out.reserve(body.size() + 64);
    // Synchronized output: supporting terminals swap the completed frame atomically.
    out += "\x1b[?2026h";
    out += "\x1b[H";
    for (char c : body) {
        if (c == '\n') {
            out += "\x1b[K\n";
        } else {
            out += c;
        }
    }
    out += "\x1b[K\x1b[J";   // clear the rest of the last line and anything below the frame
    out += "\x1b[?2026l";

    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}

int Screen::Frame() const {
    auto elapsed = std::chrono::steady_clock::now() - start_;
    return (int)(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / kFrameMs);
}

}  // namespace Tui
