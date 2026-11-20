#pragma once
#include "payment/Payment.h"

namespace shop {

class Customer;

// OOP: inheritance (hierarchical) -- Payment -> WalletPayment.
// Pays from the customer's ShopSphere wallet balance.
class WalletPayment : public Payment {
public:
    WalletPayment(const Money& amount, Customer& customer);

    std::string methodName() const override { return "ShopSphere Wallet"; }
    std::string details() const override;
    std::vector<std::string> processingSteps() const override;

protected:
    void validate() const override;
    void process() override;   // debits the wallet
    std::string referencePrefix() const override { return "WAL"; }

private:
    Customer& customer_;
};

} // namespace shop
