#include "payment/CardPayment.h"

#include <cctype>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

namespace {
std::string digitsOnly(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (std::isdigit(static_cast<unsigned char>(c))) out += c;
        else if (c != ' ' && c != '-') return "";   // reject letters etc.
    }
    return out;
}
}  // namespace

CardPayment::CardPayment(const Money& amount, const std::string& cardNumber,
                         const std::string& holder, const std::string& expiry,
                         const std::string& cvv)
    : Payment(amount), number_(digitsOnly(cardNumber)), holder_(util::trim(holder)),
      expiry_(util::trim(expiry)), cvv_(util::trim(cvv)) {}

bool CardPayment::passesLuhn(const std::string& digits) {
    if (digits.size() < 12) return false;
    int sum = 0;
    bool doubleIt = false;
    for (std::size_t i = digits.size(); i-- > 0;) {
        if (!std::isdigit(static_cast<unsigned char>(digits[i]))) return false;
        int d = digits[i] - '0';
        if (doubleIt) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        doubleIt = !doubleIt;
    }
    return sum % 10 == 0;
}

std::string CardPayment::network() const {
    if (number_.empty()) return "CARD";
    switch (number_[0]) {
        case '4': return "VISA";
        case '5': return "Mastercard";
        case '6': return "RuPay";
        default: return "CARD";
    }
}

std::string CardPayment::details() const {
    std::string last4 = number_.size() >= 4 ? number_.substr(number_.size() - 4) : number_;
    return network() + " **** " + last4;
}

void CardPayment::validate() const {
    if (number_.size() != 16) throw PaymentException("Card number must have 16 digits.");
    if (!passesLuhn(number_)) throw PaymentException("Card number failed the checksum test.");
    if (holder_.size() < 2) throw PaymentException("Please enter the name on the card.");

    // Expiry "MM/YY" must be this month or later.
    int month = 0;
    int year = 0;
    if (expiry_.size() != 5 || expiry_[2] != '/' ||
        !util::parseInt(expiry_.substr(0, 2), month) ||
        !util::parseInt(expiry_.substr(3, 2), year) || month < 1 || month > 12) {
        throw PaymentException("Expiry must be in MM/YY format.");
    }
    year += 2000;
    int nowYear = util::currentYear();
    int nowMonth = util::currentMonth();
    if (year < nowYear || (year == nowYear && month < nowMonth)) {
        throw PaymentException("This card expired in " + expiry_ + ".");
    }
    if (cvv_.size() != 3 || !util::isAllDigits(cvv_)) {
        throw PaymentException("CVV must be exactly 3 digits.");
    }
    if (number_ == declinedTestCard()) {
        throw PaymentException("Card declined by the issuing bank. Try another method.");
    }
}

std::vector<std::string> CardPayment::processingSteps() const {
    return {"Encrypting card details", "3-D Secure check with " + network(),
            "Bank approved - confirming order"};
}

} // namespace shop
