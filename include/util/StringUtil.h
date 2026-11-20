#pragma once
#include <sstream>
#include <string>
#include <vector>

// OOP: namespaces -- nested namespace shop::util for free helper functions.
namespace shop {
namespace util {

// OOP: templates -- generic conversion that works for any streamable type.
// (Used instead of std::to_string, which is missing on some older MinGW builds.)
template <typename T>
std::string toStr(const T& value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

std::string fixed(double value, int decimals);   // "4.50"

// Parsing helpers -- return false instead of throwing on bad input.
bool parseInt(const std::string& text, int& out);
bool parseDouble(const std::string& text, double& out);
int toIntOr(const std::string& text, int fallback);
double toDoubleOr(const std::string& text, double fallback);

std::string trim(const std::string& s);
std::string toLower(std::string s);
std::string toUpper(std::string s);
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);
bool startsWith(const std::string& s, const std::string& prefix);
std::vector<std::string> split(const std::string& s, char delim);
std::string join(const std::vector<std::string>& parts, const std::string& sep);
std::string repeat(const std::string& unit, int count);
std::string replaceAll(std::string s, const std::string& from, const std::string& to);

// Display-aware helpers: ignore ANSI escape codes and count UTF-8 code
// points, so boxes line up even when cells contain colours or symbols.
int displayWidth(const std::string& s);
std::string stripAnsi(const std::string& s);
std::string truncate(const std::string& s, int width);   // plain text only
// OOP: function overloading -- padRight(text, width) and padRight(text, width, fill).
std::string padRight(const std::string& s, int width);
std::string padRight(const std::string& s, int width, char fill);
std::string padLeft(const std::string& s, int width);
std::string center(const std::string& s, int width);
std::vector<std::string> wrap(const std::string& text, int width);

// Validation used by registration / payments.
bool isAllDigits(const std::string& s);
bool isValidEmail(const std::string& email);
bool isValidPhone(const std::string& phone);   // exactly 10 digits

// Password hashing (FNV-1a 64-bit, hex). Not cryptographic -- it just keeps
// plain-text passwords out of the CSV files for this course project.
std::string hashPassword(const std::string& plain);

// Clock helpers. A fixed time can be injected so demo runs are reproducible.
std::string nowStamp();                    // "2026-11-20 10:30"
void setFixedNow(const std::string& stamp);
void clearFixedNow();
int currentYear();
int currentMonth();

} // namespace util
} // namespace shop
