#pragma once
#include <string>

namespace shop {
namespace ui {

struct RGB {
    int r;
    int g;
    int b;
};

// The ShopSphere colour theme (24-bit colours).
namespace theme {
constexpr RGB kPrimary{124, 92, 255};    // violet
constexpr RGB kSecondary{0, 200, 220};   // cyan
constexpr RGB kAccent{255, 94, 168};     // pink
constexpr RGB kSuccess{46, 204, 113};
constexpr RGB kWarning{255, 183, 3};
constexpr RGB kDanger{239, 69, 101};
constexpr RGB kInfo{86, 156, 255};
constexpr RGB kText{230, 232, 240};
constexpr RGB kMuted{138, 143, 163};
constexpr RGB kFaint{88, 92, 112};
constexpr RGB kPanel{32, 28, 52};        // header / footer background
constexpr RGB kSelect{58, 48, 110};      // highlighted row background
constexpr RGB kGold{255, 200, 61};
constexpr RGB kBlack{18, 18, 24};
constexpr RGB kElectronics{56, 189, 248};
constexpr RGB kClothing{244, 114, 182};
constexpr RGB kGrocery{74, 222, 128};

RGB forCategory(const std::string& category);
}  // namespace theme

// ANSI styling helpers. Every function returns "" when colour is disabled,
// so callers never need to check Terminal::colorEnabled() themselves.
namespace style {
std::string fg(const RGB& c);
std::string bg(const RGB& c);
std::string reset();
std::string bold();
std::string dim();
std::string italic();
std::string underline();
std::string strike();

std::string paint(const std::string& text, const RGB& color);
std::string paintBold(const std::string& text, const RGB& color);
std::string muted(const std::string& text);
// Colours each visible character along a gradient a -> b (or a -> b -> c).
std::string gradient(const std::string& text, const RGB& a, const RGB& b);
std::string gradient(const std::string& text, const RGB& a, const RGB& b, const RGB& c);
std::string badge(const std::string& text, const RGB& background, const RGB& foreground);
RGB lerp(const RGB& a, const RGB& b, double t);
RGB lerp3(const RGB& a, const RGB& b, const RGB& c, double t);
}  // namespace style

}  // namespace ui
}  // namespace shop
