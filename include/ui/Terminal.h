#pragma once
#include <exception>
#include <string>

namespace shop {
namespace ui {

enum class KeyCode { None, Up, Down, Left, Right, Enter, Escape, Backspace, Tab, Char,
                     PageUp, PageDown, Home, End };

struct Key {
    KeyCode code;
    char ch;
    Key(KeyCode c = KeyCode::None, char value = 0) : code(c), ch(value) {}
    bool is(char c) const;   // case-insensitive character match
};

// Thrown when standard input is closed (EOF / Ctrl+C) so every loop can unwind cleanly.
class InputClosed : public std::exception {
public:
    const char* what() const noexcept override { return "input closed"; }
};

// Thin portable wrapper over the console:
//  * Windows: enables ENABLE_VIRTUAL_TERMINAL_PROCESSING + UTF-8 output code
//    page, reads raw keys with _getch().
//  * Linux/macOS: termios raw mode for single key presses.
// When the output is not a VT-capable terminal (or --no-color is given) every
// colour helper returns an empty string and the app degrades to plain text.
// OOP: static members -- the terminal is process-wide state.
class Terminal {
public:
    static void init(bool wantColor, bool wantAnimation);
    static void restore();

    static bool colorEnabled() { return color_; }
    static bool animationEnabled() { return animation_; }
    static bool vtEnabled() { return vt_; }
    static bool interactive() { return interactive_; }   // raw keys available
    static void setAnimationEnabled(bool on) { animation_ = on; }

    static void write(const std::string& text);
    static void flush();
    static void clear();
    static void cursorUp(int lines);
    static void clearLine();
    static void hideCursor();
    static void showCursor();

    static void sleepMs(int ms);     // skipped when animations are off
    static Key readKey();            // throws InputClosed
    static std::string readLine();   // throws InputClosed
    static std::string readSecret(); // like readLine but echoes '*'
    static int width();

private:
    static bool color_;
    static bool animation_;
    static bool vt_;
    static bool interactive_;
    static bool initialised_;
};

}  // namespace ui
}  // namespace shop
