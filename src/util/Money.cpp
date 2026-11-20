#include "util/Money.h"

#include <cstdlib>
#include <ostream>

#include "util/Exceptions.h"

namespace shop {

std::string Money::symbol_ = "Rs ";   // OOP: static member definition

long long roundToPaise(double rupees) {
    double scaled = rupees * 100.0;
    return static_cast<long long>(scaled >= 0 ? scaled + 0.5 : scaled - 0.5);
}

Money::Money(double rupees) : paise_(roundToPaise(rupees)) {}

Money Money::fromPaise(long long paise) {
    Money m;
    m.paise_ = paise;
    return m;
}

Money Money::parse(const std::string& text) {
    // Accepts "1234", "1234.5", "1234.50", "-12.30". Parsed digit by digit so
    // no precision is lost going through a double.
    std::string s;
    for (char c : text) {
        if (c != ' ' && c != ',') s += c;
    }
    if (s.empty()) throw InvalidInputException("Empty amount");
    bool negative = false;
    std::size_t i = 0;
    if (s[0] == '-') { negative = true; i = 1; }
    long long whole = 0;
    long long frac = 0;
    int fracDigits = 0;
    bool seenDot = false;
    bool seenDigit = false;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (c == '.') {
            if (seenDot) throw InvalidInputException("Invalid amount: " + text);
            seenDot = true;
        } else if (c >= '0' && c <= '9') {
            seenDigit = true;
            if (!seenDot) {
                whole = whole * 10 + (c - '0');
            } else if (fracDigits < 2) {
                frac = frac * 10 + (c - '0');
                ++fracDigits;
            }
        } else {
            throw InvalidInputException("Invalid amount: " + text);
        }
    }
    if (!seenDigit) throw InvalidInputException("Invalid amount: " + text);
    if (fracDigits == 1) frac *= 10;
    long long total = whole * 100 + frac;
    return fromPaise(negative ? -total : total);
}

Money Money::percent(double pct) const {
    return fromPaise(roundToPaise(rupees() * pct / 100.0));
}

Money Money::operator*(double factor) const {
    return fromPaise(roundToPaise(rupees() * factor));
}

void Money::setSymbol(const std::string& symbol) { symbol_ = symbol; }
const std::string& Money::symbol() { return symbol_; }

std::string Money::format(const std::string& prefix, bool keepZeroPaise) const {
    long long v = paise_ < 0 ? -paise_ : paise_;
    long long rupeesPart = v / 100;
    int paisePart = static_cast<int>(v % 100);

    // Indian grouping: last three digits, then groups of two (1,23,45,678).
    std::string digits;
    do {
        digits.insert(digits.begin(), static_cast<char>('0' + rupeesPart % 10));
        rupeesPart /= 10;
    } while (rupeesPart > 0);

    std::string grouped;
    int n = static_cast<int>(digits.size());
    for (int i = 0; i < n; ++i) {
        grouped += digits[static_cast<std::size_t>(i)];
        int remaining = n - i - 1;
        if (remaining > 0 && (remaining == 3 || (remaining > 3 && (remaining - 3) % 2 == 0))) {
            grouped += ',';
        }
    }

    std::string out = (paise_ < 0 ? "-" : "") + prefix + grouped;
    if (keepZeroPaise || paisePart != 0) {
        out += '.';
        out += static_cast<char>('0' + paisePart / 10);
        out += static_cast<char>('0' + paisePart % 10);
    }
    return out;
}

std::string Money::str() const { return format(symbol_, true); }
std::string Money::shortStr() const { return format(symbol_, false); }
std::string Money::plain() const { return format("Rs ", true); }

std::string Money::number() const {
    long long v = paise_ < 0 ? -paise_ : paise_;
    std::string whole;
    long long r = v / 100;
    do {
        whole.insert(whole.begin(), static_cast<char>('0' + r % 10));
        r /= 10;
    } while (r > 0);
    int p = static_cast<int>(v % 100);
    std::string out = (paise_ < 0 ? "-" : "") + whole + ".";
    out += static_cast<char>('0' + p / 10);
    out += static_cast<char>('0' + p % 10);
    return out;
}

// OOP: friend function -- operator<< overload for Money.
std::ostream& operator<<(std::ostream& os, const Money& m) {
    return os << m.str();
}

} // namespace shop
