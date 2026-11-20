#pragma once
#include "payment/Payment.h"

namespace shop {

// OOP: inheritance (hierarchical) -- Payment -> UpiPayment.
class UpiPayment : public Payment {
public:
    UpiPayment(const Money& amount, const std::string& upiId);

    std::string methodName() const override { return "UPI"; }
    std::string details() const override { return upiId_; }
    std::vector<std::string> processingSteps() const override;

    static bool isValidUpiId(const std::string& id);
    static Money perTransactionLimit() { return Money(100000.0); }

protected:
    void validate() const override;
    std::string referencePrefix() const override { return "UPI"; }

private:
    std::string upiId_;
};

} // namespace shop
