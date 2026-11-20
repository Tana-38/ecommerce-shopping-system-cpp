#pragma once
#include "payment/Payment.h"

namespace shop {

// OOP: inheritance (hierarchical) -- Payment -> CardPayment.
class CardPayment : public Payment {
public:
    CardPayment(const Money& amount, const std::string& cardNumber, const std::string& holder,
                const std::string& expiry, const std::string& cvv);

    std::string methodName() const override { return "Card"; }
    std::string details() const override;   // "VISA **** 1111"
    std::vector<std::string> processingSteps() const override;

    std::string network() const;
    static bool passesLuhn(const std::string& digits);
    // Luhn-valid number that the "bank" always declines (for demos/tests).
    static const char* declinedTestCard() { return "4000000000000002"; }

protected:
    void validate() const override;
    std::string referencePrefix() const override { return "CARD"; }

private:
    std::string number_;   // digits only
    std::string holder_;
    std::string expiry_;   // MM/YY
    std::string cvv_;
};

} // namespace shop
