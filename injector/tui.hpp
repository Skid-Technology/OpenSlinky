// The loader's terminal UI: the four-frame Slinky logo and the sliding
// underscore line, ported from the Go loader (animation/animation.go) so the
// injector looks and moves exactly like it.
#pragma once

#include <chrono>
#include <string>

namespace Tui {

bool EnableAnsi();          // Windows: turn on VT processing; Linux: no-op
bool StdoutIsTty();

std::string Logo(int frame);
std::string ErrorLogo();
std::string SlinkyLine(int frame);

class Screen {
public:
    explicit Screen(bool animate);
    ~Screen();

    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    void Render(const std::string& title, const std::string& message, bool error = false);
private:
    // The animation advances with time, not with redraws: the caller redraws every ~16 ms so status
    // messages show up at once, which made a frame-per-redraw animation run at 60 fps.
    static constexpr int kFrameMs = 100;
    int Frame() const;

    bool animate_;
    std::chrono::steady_clock::time_point start_ = std::chrono::steady_clock::now();
    // Last rendered content: the caller redraws at 60 Hz, but the frame only changes every
    // kFrameMs (or on a status change), so identical redraws are skipped.
    std::string last_body_;
};

}  // namespace Tui
