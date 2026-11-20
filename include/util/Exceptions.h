#pragma once
#include <stdexcept>
#include <string>

namespace shop {

// OOP: exception handling -- custom exception hierarchy rooted at ShopException.
// Every error the store can raise derives from ShopException, so the UI can
// catch the base class once and still show a precise, typed message.
class ShopException : public std::runtime_error {
public:
    explicit ShopException(const std::string& message,
                           const std::string& code = "SHOP")
        : std::runtime_error(message), code_(code) {}
    virtual ~ShopException() = default;

    // OOP: runtime polymorphism -- each subclass reports its own title.
    virtual std::string title() const { return "Store error"; }
    const std::string& code() const { return code_; }

private:
    std::string code_;
};

// OOP: inheritance (hierarchical) -- all of these specialise ShopException.
class OutOfStockException : public ShopException {
public:
    OutOfStockException(const std::string& product, int requested, int available)
        : ShopException("Only " + numberText(available) + " unit(s) of '" + product +
                            "' left, but " + numberText(requested) + " requested.",
                        "STOCK"),
          requested_(requested), available_(available) {}
    std::string title() const override { return "Out of stock"; }
    int requested() const { return requested_; }
    int available() const { return available_; }

private:
    int requested_;
    int available_;
    static std::string numberText(int n);
};

class InvalidInputException : public ShopException {
public:
    explicit InvalidInputException(const std::string& message)
        : ShopException(message, "INPUT") {}
    std::string title() const override { return "Invalid input"; }
};

class PaymentException : public ShopException {
public:
    explicit PaymentException(const std::string& message)
        : ShopException(message, "PAYMENT") {}
    std::string title() const override { return "Payment failed"; }
};

class AuthException : public ShopException {
public:
    explicit AuthException(const std::string& message)
        : ShopException(message, "AUTH") {}
    std::string title() const override { return "Authentication failed"; }
};

class CouponException : public ShopException {
public:
    explicit CouponException(const std::string& message)
        : ShopException(message, "COUPON") {}
    std::string title() const override { return "Coupon not applied"; }
};

class NotFoundException : public ShopException {
public:
    explicit NotFoundException(const std::string& message)
        : ShopException(message, "NOT_FOUND") {}
    std::string title() const override { return "Not found"; }
};

class FileException : public ShopException {
public:
    explicit FileException(const std::string& message)
        : ShopException(message, "FILE") {}
    std::string title() const override { return "File error"; }
};

inline std::string OutOfStockException::numberText(int n) {
    // Small local helper so this header stays independent of StringUtil.
    std::string digits;
    bool negative = n < 0;
    long long v = negative ? -static_cast<long long>(n) : n;
    do {
        digits.insert(digits.begin(), static_cast<char>('0' + v % 10));
        v /= 10;
    } while (v > 0);
    return negative ? "-" + digits : digits;
}

} // namespace shop
