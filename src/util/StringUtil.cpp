#include "util/StringUtil.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <iomanip>

namespace shop {
namespace util {

std::string fixed(double value, int decimals) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(decimals) << value;
    return os.str();
}

bool parseInt(const std::string& text, int& out) {
    std::string t = trim(text);
    if (t.empty()) return false;
    char* end = nullptr;
    errno = 0;
    long v = std::strtol(t.c_str(), &end, 10);
    if (errno != 0 || end == t.c_str() || *end != '\0') return false;
    if (v > 2147483647L || v < -2147483647L) return false;
    out = static_cast<int>(v);
    return true;
}

bool parseDouble(const std::string& text, double& out) {
    std::string t = trim(text);
    if (t.empty()) return false;
    char* end = nullptr;
    errno = 0;
    double v = std::strtod(t.c_str(), &end);
    if (errno != 0 || end == t.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

int toIntOr(const std::string& text, int fallback) {
    int v = 0;
    return parseInt(text, v) ? v : fallback;
}

double toDoubleOr(const std::string& text, double fallback) {
    double v = 0;
    return parseDouble(text, v) ? v : fallback;
}

std::string trim(const std::string& s) {
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == delim) {
            parts.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    parts.push_back(cur);
    return parts;
}

std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) out += sep;
        out += parts[i];
    }
    return out;
}

std::string repeat(const std::string& unit, int count) {
    std::string out;
    for (int i = 0; i < count; ++i) out += unit;
    return out;
}

std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::string stripAnsi(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
            i += 2;
            while (i < s.size() && !(s[i] >= '@' && s[i] <= '~')) ++i;
            continue;   // skips the final byte too
        }
        out += s[i];
    }
    return out;
}

int displayWidth(const std::string& s) {
    std::string plain = stripAnsi(s);
    int width = 0;
    for (unsigned char c : plain) {
        if ((c & 0xC0) != 0x80) ++width;   // count UTF-8 lead bytes only
    }
    return width;
}

std::string truncate(const std::string& s, int width) {
    if (displayWidth(s) <= width) return s;
    if (width <= 1) return width == 1 ? "." : "";
    // Walk code points until width-1 columns are used, then add an ellipsis.
    std::string out;
    int used = 0;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        std::size_t len = 1;
        if (c >= 0xF0) len = 4;
        else if (c >= 0xE0) len = 3;
        else if (c >= 0xC0) len = 2;
        if (used + 1 > width - 1) break;
        out += s.substr(i, len);
        ++used;
        i += len;
    }
    return out + "\xE2\x80\xA6";   // U+2026 horizontal ellipsis
}

std::string padRight(const std::string& s, int width) {
    return padRight(s, width, ' ');
}

std::string padRight(const std::string& s, int width, char fill) {
    int w = displayWidth(s);
    return w >= width ? s : s + std::string(static_cast<std::size_t>(width - w), fill);
}

std::string padLeft(const std::string& s, int width) {
    int w = displayWidth(s);
    return w >= width ? s : std::string(static_cast<std::size_t>(width - w), ' ') + s;
}

std::string center(const std::string& s, int width) {
    int w = displayWidth(s);
    if (w >= width) return s;
    int left = (width - w) / 2;
    int right = width - w - left;
    return std::string(static_cast<std::size_t>(left), ' ') + s +
           std::string(static_cast<std::size_t>(right), ' ');
}

std::vector<std::string> wrap(const std::string& text, int width) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string word;
    std::string line;
    while (in >> word) {
        if (!line.empty() && displayWidth(line) + 1 + displayWidth(word) > width) {
            lines.push_back(line);
            line.clear();
        }
        line += (line.empty() ? "" : " ") + word;
    }
    if (!line.empty()) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    return lines;
}

bool isAllDigits(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(),
                       [](unsigned char c) { return std::isdigit(c) != 0; });
}

bool isValidEmail(const std::string& email) {
    // name@domain.tld : one '@', something before it, a dot after it,
    // no spaces, and the dot is neither first nor last in the domain.
    if (email.find(' ') != std::string::npos) return false;
    std::size_t at = email.find('@');
    if (at == std::string::npos || at == 0) return false;
    if (email.find('@', at + 1) != std::string::npos) return false;
    std::string domain = email.substr(at + 1);
    std::size_t dot = domain.rfind('.');
    return dot != std::string::npos && dot > 0 && dot + 1 < domain.size();
}

bool isValidPhone(const std::string& phone) {
    return phone.size() == 10 && isAllDigits(phone);
}

std::string hashPassword(const std::string& plain) {
    unsigned long long h = 1469598103934665603ULL;   // FNV offset basis
    for (unsigned char c : plain) {
        h ^= c;
        h *= 1099511628211ULL;                       // FNV prime
    }
    static const char* hex = "0123456789abcdef";
    std::string out(16, '0');
    for (int i = 15; i >= 0; --i) {
        out[static_cast<std::size_t>(i)] = hex[h & 0xF];
        h >>= 4;
    }
    return out;
}

namespace {
std::string& fixedNow() {
    static std::string value;   // empty = use the real clock
    return value;
}
}  // namespace

std::string nowStamp() {
    if (!fixedNow().empty()) return fixedNow();
    std::time_t t = std::time(nullptr);
    std::tm* local = std::localtime(&t);
    char buf[32] = {0};
    if (local) std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", local);
    return buf;
}

void setFixedNow(const std::string& stamp) { fixedNow() = stamp; }
void clearFixedNow() { fixedNow().clear(); }

int currentYear() { return toIntOr(nowStamp().substr(0, 4), 2026); }
int currentMonth() { return toIntOr(nowStamp().substr(5, 2), 1); }

} // namespace util
} // namespace shop
