#include "ui/Terminal.h"

#include <cctype>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#endif

namespace shop {
namespace ui {

bool Terminal::color_ = false;
bool Terminal::animation_ = false;
bool Terminal::vt_ = false;
bool Terminal::interactive_ = false;
bool Terminal::initialised_ = false;

bool Key::is(char c) const {
    return code == KeyCode::Char &&
           std::tolower(static_cast<unsigned char>(ch)) == std::tolower(static_cast<unsigned char>(c));
}

namespace {
#ifdef _WIN32
DWORD g_originalOutMode = 0;
bool g_haveOutMode = false;
UINT g_originalCodePage = 0;
#endif

void onSignal(int) {
    // Leave the terminal usable if the user presses Ctrl+C mid-animation.
    std::fputs("\x1b[0m\x1b[?25h\n", stdout);
    std::fflush(stdout);
    std::_Exit(130);
}
}  // namespace

void Terminal::init(bool wantColor, bool wantAnimation) {
    const char* term = std::getenv("TERM");
    bool termLooksVt = term != nullptr && std::string(term) != "dumb";
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) {
        g_originalOutMode = mode;
        g_haveOutMode = true;
        vt_ = SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        g_originalCodePage = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
        // Binary mode stops the C runtime from splitting UTF-8 sequences while
        // translating "\n" (the console still treats LF as a full newline).
        _setmode(1, _O_BINARY);
    } else {
        // Not a console (pipe/file, or a mintty without winpty): trust TERM.
        vt_ = termLooksVt;
    }
    DWORD inMode = 0;
    interactive_ = GetConsoleMode(in, &inMode) != 0 && g_haveOutMode;
#else
    vt_ = isatty(STDOUT_FILENO) && termLooksVt;
    interactive_ = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
    // Fully buffer stdout: each frame leaves in one write. Every place that waits
    // (key reads, line reads, sleeps) flushes first, so nothing is held back.
    static char outBuffer[1 << 16];
    std::setvbuf(stdout, outBuffer, _IOFBF, sizeof(outBuffer));
    color_ = wantColor && vt_ && std::getenv("NO_COLOR") == nullptr;
    // Animations only make sense when we can redraw in place.
    animation_ = wantAnimation && vt_;
    initialised_ = true;
    std::signal(SIGINT, onSignal);
}

void Terminal::restore() {
    if (!initialised_) return;
    if (vt_) write("\x1b[0m\x1b[?25h");
    flush();
#ifdef _WIN32
    if (g_haveOutMode) {
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), g_originalOutMode);
        if (g_originalCodePage != 0) SetConsoleOutputCP(g_originalCodePage);
    }
#endif
    initialised_ = false;
}

void Terminal::write(const std::string& text) { std::cout << text; }
void Terminal::flush() { std::cout.flush(); }

void Terminal::clear() {
    if (vt_) {
        write("\x1b[2J\x1b[3J\x1b[H");
    } else {
        write("\n");
    }
    flush();
}

void Terminal::cursorUp(int lines) {
    if (vt_ && lines > 0) {
        std::string n;
        int v = lines;
        while (v > 0) {
            n.insert(n.begin(), static_cast<char>('0' + v % 10));
            v /= 10;
        }
        write("\x1b[" + n + "F");   // up N lines, to column 1
    }
}

void Terminal::clearLine() {
    if (vt_) write("\x1b[2K\r");
}

void Terminal::hideCursor() {
    if (vt_) write("\x1b[?25l");
}

void Terminal::showCursor() {
    if (vt_) write("\x1b[?25h");
}

void Terminal::sleepMs(int ms) {
    if (!animation_ || ms <= 0) return;
    flush();
#ifdef _WIN32
    Sleep(static_cast<DWORD>(ms));
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = static_cast<long>(ms % 1000) * 1000000L;
    nanosleep(&ts, nullptr);
#endif
}

namespace {
Key readPipedKey() {
    for (;;) {
        int c = std::cin.get();
        if (c == EOF) throw InputClosed();
        if (c == '\r') continue;
        if (c == '\n') return Key(KeyCode::Enter);
        if (c == 27) return Key(KeyCode::Escape);
        if (c == '\t') return Key(KeyCode::Tab);
        return Key(KeyCode::Char, static_cast<char>(c));
    }
}

#ifndef _WIN32
struct RawMode {
    termios saved;
    bool ok;
    RawMode() : ok(false) {
        if (tcgetattr(STDIN_FILENO, &saved) == 0) {
            termios raw = saved;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            ok = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
    }
    ~RawMode() {
        if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    }
};

bool byteReady(int timeoutMs) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeoutMs * 1000;
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
}

int readByte() {
    unsigned char c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    return n == 1 ? c : -1;
}
#endif
}  // namespace

Key Terminal::readKey() {
    flush();
    if (!interactive_) return readPipedKey();
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        switch (c2) {
            case 72: return Key(KeyCode::Up);
            case 80: return Key(KeyCode::Down);
            case 75: return Key(KeyCode::Left);
            case 77: return Key(KeyCode::Right);
            case 73: return Key(KeyCode::PageUp);
            case 81: return Key(KeyCode::PageDown);
            case 71: return Key(KeyCode::Home);
            case 79: return Key(KeyCode::End);
            default: return Key(KeyCode::None);
        }
    }
    if (c == 3) throw InputClosed();   // Ctrl+C
    if (c == 13 || c == 10) return Key(KeyCode::Enter);
    if (c == 27) return Key(KeyCode::Escape);
    if (c == 8) return Key(KeyCode::Backspace);
    if (c == 9) return Key(KeyCode::Tab);
    return Key(KeyCode::Char, static_cast<char>(c));
#else
    RawMode raw;
    int c = readByte();
    if (c < 0 || c == 4) throw InputClosed();   // EOF / Ctrl+D
    if (c == 27) {
        if (!byteReady(40)) return Key(KeyCode::Escape);
        int c2 = readByte();
        if (c2 != '[' && c2 != 'O') return Key(KeyCode::Escape);
        int c3 = readByte();
        switch (c3) {
            case 'A': return Key(KeyCode::Up);
            case 'B': return Key(KeyCode::Down);
            case 'C': return Key(KeyCode::Right);
            case 'D': return Key(KeyCode::Left);
            case 'H': return Key(KeyCode::Home);
            case 'F': return Key(KeyCode::End);
            case '5': readByte(); return Key(KeyCode::PageUp);
            case '6': readByte(); return Key(KeyCode::PageDown);
            default: return Key(KeyCode::None);
        }
    }
    if (c == '\n' || c == '\r') return Key(KeyCode::Enter);
    if (c == 127 || c == 8) return Key(KeyCode::Backspace);
    if (c == '\t') return Key(KeyCode::Tab);
    return Key(KeyCode::Char, static_cast<char>(c));
#endif
}

std::string Terminal::readLine() {
    flush();
    std::string line;
    if (!std::getline(std::cin, line)) throw InputClosed();
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
    if (!interactive_) write(line + "\n");   // echo piped input so transcripts read well
    return line;
}

std::string Terminal::readSecret() {
    if (!interactive_) {
        flush();
        std::string line;
        if (!std::getline(std::cin, line)) throw InputClosed();
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
        write(std::string(line.size(), '*') + "\n");
        return line;
    }
    std::string secret;
    for (;;) {
        Key k = readKey();
        if (k.code == KeyCode::Enter) break;
        if (k.code == KeyCode::Escape) {
            secret.clear();
            break;
        }
        if (k.code == KeyCode::Backspace) {
            if (!secret.empty()) {
                secret.erase(secret.size() - 1);
                write("\b \b");
            }
        } else if (k.code == KeyCode::Char && std::isprint(static_cast<unsigned char>(k.ch))) {
            secret += k.ch;
            write("*");
        }
        flush();
    }
    write("\n");
    return secret;
}

int Terminal::width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        return info.srWindow.Right - info.srWindow.Left + 1;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
#endif
    return 100;
}

}  // namespace ui
}  // namespace shop
