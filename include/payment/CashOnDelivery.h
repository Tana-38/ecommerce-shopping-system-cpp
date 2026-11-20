#pragma once
#include "payment/Payment.h"

namespace shop {

// OOP: inheritance (hierarchical) -- Payment -> CashOnDelivery.
class CashOnDelivery : public Payment {
public:
    explicit CashOnDelivery(const Money& amount);

    std::string methodName() const override { return "Cash on Delivery"; }
    std::string details() const override { return "Pay at your doorstep"; }
    bool isPrepaid() const override { return false; }
    std::vector<std::string> processingSteps() const override;

    static Money limit() { return Money(50000.0); }

protected:
    void validate() const override;
    std::string referencePrefix() const override { return "COD"; }
};

} // namespace shop
