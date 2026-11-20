#include "payment/WalletPayment.h"

#include "core/Customer.h"
#include "util/Exceptions.h"

namespace shop {

WalletPayment::WalletPayment(const Money& amount, Customer& customer)
    : Payment(amount), customer_(customer) {}

std::string WalletPayment::details() const {
    return "Balance " + customer_.getWallet().str();
}

void WalletPayment::validate() const {
    if (customer_.isGuest()) throw PaymentException("Guests do not have a wallet.");
    if (customer_.getWallet() < amount()) {
        throw PaymentException("Wallet balance " + customer_.getWallet().str() +
                               " is less than " + amount().str() + ".");
    }
}

void WalletPayment::process() { customer_.debitWallet(amount()); }

std::vector<std::string> WalletPayment::processingSteps() const {
    return {"Verifying wallet balance", "Debiting " + amount().str() + " from wallet",
            "Payment complete - confirming order"};
}

} // namespace shop
