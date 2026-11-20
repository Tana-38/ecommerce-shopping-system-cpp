#include "ui/Color.h"

#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {

RGB theme::forCategory(const std::string& category) {
    if (category == "Electronics" || category == "Smartphone") return kElectronics;
    if (category == "Clothing") return kClothing;
    if (category == "Grocery") return kGrocery;
    return kMuted;
}

namespace style {

namespace {
std::string triple(const RGB& c) {
    return util::toStr(c.r) + ";" + util::toStr(c.g) + ";" + util::toStr(c.b);
}
bool on() { return Terminal::colorEnabled(); }
}  // namespace

std::string fg(const RGB& c) { return on() ? "\x1b[38;2;" + triple(c) + "m" : ""; }
std::string bg(const RGB& c) { return on() ? "\x1b[48;2;" + triple(c) + "m" : ""; }
std::string reset() { return on() ? "\x1b[0m" : ""; }
std::string bold() { return on() ? "\x1b[1m" : ""; }
std::string dim() { return on() ? "\x1b[2m" : ""; }
std::string italic() { return on() ? "\x1b[3m" : ""; }
std::string underline() { return on() ? "\x1b[4m" : ""; }
std::string strike() { return on() ? "\x1b[9m" : ""; }

std::string paint(const std::string& text, const RGB& color) {
    return on() ? fg(color) + text + reset() : text;
}

std::string paintBold(const std::string& text, const RGB& color) {
    return on() ? bold() + fg(color) + text + reset() : text;
}

std::string muted(const std::string& text) { return paint(text, theme::kMuted); }

RGB lerp(const RGB& a, const RGB& b, double t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    RGB out;
    out.r = static_cast<int>(a.r + (b.r - a.r) * t + 0.5);
    out.g = static_cast<int>(a.g + (b.g - a.g) * t + 0.5);
    out.b = static_cast<int>(a.b + (b.b - a.b) * t + 0.5);
    return out;
}

RGB lerp3(const RGB& a, const RGB& b, const RGB& c, double t) {
    if (t < 0.5) return lerp(a, b, t * 2.0);
    return lerp(b, c, (t - 0.5) * 2.0);
}

namespace {
// Splits a UTF-8 string into code points (one string each).
std::size_t codePointLength(unsigned char c) {
    if (c >= 0xF0) return 4;
    if (c >= 0xE0) return 3;
    if (c >= 0xC0) return 2;
    return 1;
}

template <typename ColorAt>
std::string gradientWith(const std::string& text, ColorAt colorAt) {
    if (!on()) return text;
    int total = util::displayWidth(text);
    if (total <= 0) return text;
    std::string out;
    int index = 0;
    for (std::size_t i = 0; i < text.size();) {
        std::size_t len = codePointLength(static_cast<unsigned char>(text[i]));
        std::string cp = text.substr(i, len);
        if (cp != " ") {
            double t = total > 1 ? static_cast<double>(index) / (total - 1) : 0.0;
            out += fg(colorAt(t));
        }
        out += cp;
        ++index;
        i += len;
    }
    return out + reset();
}
}  // namespace

std::string gradient(const std::string& text, const RGB& a, const RGB& b) {
    return gradientWith(text, [&](double t) { return lerp(a, b, t); });
}

std::string gradient(const std::string& text, const RGB& a, const RGB& b, const RGB& c) {
    return gradientWith(text, [&](double t) { return lerp3(a, b, c, t); });
}

std::string badge(const std::string& text, const RGB& background, const RGB& foreground) {
    if (!on()) return "[" + text + "]";
    return bg(background) + fg(foreground) + bold() + " " + text + " " + reset();
}

}  // namespace style
}  // namespace ui
}  // namespace shop
