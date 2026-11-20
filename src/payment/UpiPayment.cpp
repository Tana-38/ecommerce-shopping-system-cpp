#include "payment/UpiPayment.h"

#include <cctype>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

UpiPayment::UpiPayment(const Money& amount, const std::string& upiId)
    : Payment(amount), upiId_(util::toLower(util::trim(upiId))) {}

bool UpiPayment::isValidUpiId(const std::string& id) {
    // handle@bank : handle = 2+ of [a-z0-9._-], bank = 2+ letters.
    std::size_t at = id.find('@');
    if (at == std::string::npos || at < 2 || at + 3 > id.size()) return false;
    if (id.find('@', at + 1) != std::string::npos) return false;
    for (std::size_t i = 0; i < at; ++i) {
        unsigned char c = static_cast<unsigned char>(id[i]);
        if (!(std::isalnum(c) || c == '.' || c == '_' || c == '-')) return false;
    }
    for (std::size_t i = at + 1; i < id.size(); ++i) {
        if (!std::isalpha(static_cast<unsigned char>(id[i]))) return false;
    }
    return true;
}

void UpiPayment::validate() const {
    if (!isValidUpiId(upiId_)) {
        throw PaymentException("'" + upiId_ + "' is not a valid UPI ID (expected name@bank).");
    }
    if (amount() > perTransactionLimit()) {
        throw PaymentException("UPI payments are limited to " + perTransactionLimit().str() +
                               " per transaction.");
    }
}

std::vector<std::string> UpiPayment::processingSteps() const {
    return {"Sending collect request to " + upiId_, "Waiting for UPI PIN approval",
            "Payment received - confirming order"};
}

} // namespace shop
