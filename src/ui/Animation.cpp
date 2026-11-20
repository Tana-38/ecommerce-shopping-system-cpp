#include "ui/Animation.h"

#include <algorithm>
#include <utility>

#include "ui/Chart.h"
#include "ui/Color.h"
#include "ui/Table.h"
#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {
namespace anim {

namespace {
std::string pad(int n) { return std::string(static_cast<std::size_t>(n > 0 ? n : 0), ' '); }

bool animating() { return Terminal::animationEnabled() && Terminal::vtEnabled(); }

// Spinner frame: a dot that bounces across four positions.
std::string spinnerFrame(int frame) {
    static const int path[6] = {0, 1, 2, 3, 2, 1};
    int pos = path[frame % 6];
    std::string s;
    for (int i = 0; i < 4; ++i) {
        if (i == pos) {
            s += style::paint("●", style::lerp(theme::kPrimary, theme::kSecondary, i / 3.0));
        } else {
            s += style::paint("∙", theme::kFaint);
        }
    }
    return s;
}

// A grid of "pixels" (each 2 characters wide) drawn in a path order.
typedef std::vector<std::pair<int, int>> Path;

std::vector<std::string> drawGrid(const Path& path, std::size_t count, int rows, int cols,
                                  const RGB& from, const RGB& to, int shift) {
    std::vector<std::string> grid(static_cast<std::size_t>(rows));
    std::vector<std::vector<int>> cell(static_cast<std::size_t>(rows),
                                       std::vector<int>(static_cast<std::size_t>(cols), -1));
    for (std::size_t i = 0; i < count && i < path.size(); ++i) {
        cell[static_cast<std::size_t>(path[i].first)][static_cast<std::size_t>(path[i].second)] =
            static_cast<int>(i);
    }
    for (int r = 0; r < rows; ++r) {
        std::string line = pad(shift);
        for (int c = 0; c < cols; ++c) {
            int idx = cell[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (idx < 0) {
                line += "  ";
            } else {
                double t = path.size() > 1 ? static_cast<double>(idx) / (path.size() - 1) : 0;
                line += style::paint(Terminal::colorEnabled() ? "██" : "##", style::lerp(from, to, t));
            }
        }
        grid[static_cast<std::size_t>(r)] = line;
    }
    return grid;
}

void printBlock(const std::vector<std::string>& lines, int indent) {
    for (const std::string& l : lines) {
        Terminal::clearLine();
        Terminal::write(pad(indent) + l + "\n");
    }
}

void banner(const std::string& title, const std::string& subtitle, const RGB& color) {
    std::vector<std::string> content = {style::paintBold(title, color)};
    if (!subtitle.empty()) content.push_back(style::muted(subtitle));
    int w = std::max(util::displayWidth(title), util::displayWidth(subtitle)) + 4;
    std::vector<std::string> centered;
    for (const std::string& c : content) centered.push_back(util::center(c, w));
    int indent = std::max(2, (96 - (w + 4)) / 2);   // centre inside the 96-column layout
    Terminal::write(box(centered, w, "", color, indent));
}
}  // namespace

void pause(int ms) { Terminal::sleepMs(ms); }

void typewriter(const std::string& text, int msPerChar, int indent) {
    Terminal::write(pad(indent));
    if (!animating()) {
        Terminal::write(text + "\n");
        Terminal::flush();
        return;
    }
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == '\x1b') {   // emit a whole escape sequence at once
            std::size_t j = i + 1;
            while (j < text.size() && !(text[j] >= '@' && text[j] <= '~' && j > i + 1)) ++j;
            Terminal::write(text.substr(i, j - i + 1));
            i = j + 1;
            continue;
        }
        unsigned char c = static_cast<unsigned char>(text[i]);
        std::size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        Terminal::write(text.substr(i, len));
        i += len;
        Terminal::sleepMs(text[i - 1] == ' ' ? msPerChar / 2 : msPerChar);
    }
    Terminal::write("\n");
    Terminal::flush();
}

void spinner(const std::string& label, int durationMs, int indent) {
    if (animating()) {
        Terminal::hideCursor();
        int frames = durationMs / 70;
        for (int f = 0; f < frames; ++f) {
            Terminal::write("\r" + pad(indent) + spinnerFrame(f) + "  " +
                            style::paint(label, theme::kText) + style::muted(" ..."));
            Terminal::flush();
            Terminal::sleepMs(70);
        }
        Terminal::clearLine();
        Terminal::showCursor();
    }
    Terminal::write(pad(indent) + style::paintBold(" ✔  ", theme::kSuccess) +
                    style::paint(label, theme::kText) + "\n");
    Terminal::flush();
}

void steps(const std::vector<std::string>& labels, int msEach, int indent) {
    for (const std::string& l : labels) spinner(l, msEach, indent);
}

void progressBar(const std::string& label, int durationMs, int width, int indent) {
    const int frames = animating() ? std::max(1, durationMs / 30) : 1;
    if (animating()) Terminal::hideCursor();
    for (int f = 1; f <= frames; ++f) {
        double t = static_cast<double>(f) / frames;
        // Ease-out so the bar slows down near the end.
        double eased = 1.0 - (1.0 - t) * (1.0 - t);
        int pct = static_cast<int>(eased * 100 + 0.5);
        std::string line = pad(indent) + style::paint(util::padRight(label, 18), theme::kText) + " " +
                           meter(eased, width, theme::kPrimary, theme::kSecondary) + " " +
                           style::paintBold(util::padLeft(util::toStr(pct) + "%", 4), theme::kSecondary);
        Terminal::write("\r" + line);
        Terminal::flush();
        if (f < frames) Terminal::sleepMs(30);
    }
    Terminal::write("\n");
    if (animating()) Terminal::showCursor();
    Terminal::flush();
}

void successBurst(const std::string& title, const std::string& subtitle) {
    // Check mark on an 8x6 pixel grid: short stroke down, long stroke up.
    const Path path = {{3, 0}, {4, 1}, {5, 2}, {4, 3}, {3, 4}, {2, 5}, {1, 6}, {0, 7}};
    const int rows = 6;
    const int indent = 38;
    if (animating()) {
        Terminal::hideCursor();
        for (std::size_t k = 1; k <= path.size(); ++k) {
            if (k > 1) Terminal::cursorUp(rows);
            printBlock(drawGrid(path, k, rows, 8, theme::kSuccess, theme::kSecondary, 0), indent);
            Terminal::flush();
            Terminal::sleepMs(55);
        }
        Terminal::showCursor();
    } else {
        printBlock(drawGrid(path, path.size(), rows, 8, theme::kSuccess, theme::kSecondary, 0),
                   indent);
    }
    Terminal::write("\n");
    banner("✔  " + title, subtitle, theme::kSuccess);
    Terminal::flush();
}

void failureShake(const std::string& title, const std::string& subtitle) {
    const Path path = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5},
                       {0, 5}, {1, 4}, {2, 3}, {3, 2}, {4, 1}, {5, 0}};
    const int rows = 6;
    const int indent = 38;
    if (animating()) {
        Terminal::hideCursor();
        printBlock(drawGrid(path, path.size(), rows, 6, theme::kDanger, theme::kAccent, 0), indent);
        const int shifts[8] = {3, 0, 3, 0, 2, 0, 1, 0};
        for (int s : shifts) {
            Terminal::sleepMs(45);
            Terminal::cursorUp(rows);
            printBlock(drawGrid(path, path.size(), rows, 6, theme::kDanger, theme::kAccent, s),
                       indent - 1);
        }
        Terminal::showCursor();
    } else {
        printBlock(drawGrid(path, path.size(), rows, 6, theme::kDanger, theme::kAccent, 0), indent);
    }
    Terminal::write("\n");
    banner("✘  " + title, subtitle, theme::kDanger);
    Terminal::flush();
}

void wipe() {
    if (!animating() || !Terminal::colorEnabled()) {
        Terminal::clear();
        return;
    }
    Terminal::hideCursor();
    Terminal::clear();
    const int width = 96;
    for (int step = 0; step <= 4; ++step) {
        std::string line = "\r  ";
        for (int i = 0; i < width; ++i) {
            double t = static_cast<double>(i) / (width - 1);
            line += style::fg(style::lerp(theme::kPrimary, theme::kAccent, t)) +
                    (i < width * (step + 1) / 5 ? "━" : " ");
        }
        Terminal::write(line + style::reset());
        Terminal::flush();
        Terminal::sleepMs(18);
    }
    Terminal::clear();
    Terminal::showCursor();
}

}  // namespace anim
}  // namespace ui
}  // namespace shop
