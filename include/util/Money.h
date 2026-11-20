#pragma once
#include <iosfwd>
#include <string>

namespace shop {

// Money stores an amount as whole paise (1 rupee = 100 paise) in a 64-bit
// integer, so cart totals never suffer from floating-point drift.
// OOP: classes & objects, encapsulation, compile-time polymorphism (operator overloading).
class Money {
public:
    // OOP: constructors -- default and parameterised.
    Money() : paise_(0) {}
    explicit Money(double rupees);
    // OOP: copy constructor (trivial member-wise copy is exactly right here).
    Money(const Money& other) = default;
    Money& operator=(const Money& other) = default;

    static Money fromPaise(long long paise);
    static Money parse(const std::string& text);   // "1234.50" -> Rs 1234.50

    long long paise() const { return paise_; }     // OOP: const correctness
    double rupees() const { return static_cast<double>(paise_) / 100.0; }
    bool isZero() const { return paise_ == 0; }

    // Returns pct% of this amount, rounded to the nearest paisa.
    Money percent(double pct) const;

    std::string str() const;        // "<symbol>1,23,456.50" (Indian digit grouping)
    std::string shortStr() const;   // drops ".00" for whole rupees
    std::string plain() const;      // always "Rs 1,23,456.50" (files, invoices)
    std::string number() const;     // "123456.50" (CSV)

    // OOP: operator overloading -- arithmetic.
    Money operator+(const Money& rhs) const { return fromPaise(paise_ + rhs.paise_); }
    Money operator-(const Money& rhs) const { return fromPaise(paise_ - rhs.paise_); }
    Money operator*(int qty) const { return fromPaise(paise_ * qty); }
    Money operator*(double factor) const;   // OOP: function overloading (int vs double)
    Money& operator+=(const Money& rhs) { paise_ += rhs.paise_; return *this; }
    Money& operator-=(const Money& rhs) { paise_ -= rhs.paise_; return *this; }

    // OOP: operator overloading -- comparisons.
    bool operator==(const Money& rhs) const { return paise_ == rhs.paise_; }
    bool operator!=(const Money& rhs) const { return paise_ != rhs.paise_; }
    bool operator<(const Money& rhs) const { return paise_ < rhs.paise_; }
    bool operator>(const Money& rhs) const { return paise_ > rhs.paise_; }
    bool operator<=(const Money& rhs) const { return paise_ <= rhs.paise_; }
    bool operator>=(const Money& rhs) const { return paise_ >= rhs.paise_; }

    // OOP: static members -- one currency symbol shared by every Money object.
    static void setSymbol(const std::string& symbol);
    static const std::string& symbol();

    // OOP: friend function -- operator<< reads the private paise_ directly.
    friend std::ostream& operator<<(std::ostream& os, const Money& m);

private:
    long long paise_;
    static std::string symbol_;

    std::string format(const std::string& prefix, bool keepZeroPaise) const;
};

// Round a rupee amount to the nearest paisa (portable, no std::llround needed).
long long roundToPaise(double rupees);

} // namespace shop
