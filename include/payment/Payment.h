#pragma once
#include <string>
#include <vector>

#include "util/Money.h"

namespace shop {

// OOP: abstraction -- Payment is an abstract base class. Checkout code only
// ever talks to a Payment& / unique_ptr<Payment>; the concrete method (UPI,
// card, cash on delivery, wallet) is chosen at runtime.
class Payment {
public:
    explicit Payment(const Money& amount);
    // OOP: destructor -- virtual for safe deletion through unique_ptr<Payment>.
    virtual ~Payment();
    Payment(const Payment&) = delete;              // a payment is a one-off event
    Payment& operator=(const Payment&) = delete;

    // Template method (non-virtual): validate -> process -> issue reference.
    // Throws PaymentException on any failure; returns the transaction reference.
    std::string pay();

    // OOP: abstraction -- every payment method must describe itself.
    virtual std::string methodName() const = 0;     // "UPI", "Card", ...
    virtual std::string details() const = 0;        // masked, safe to print
    virtual bool isPrepaid() const { return true; }
    // Messages shown by the UI spinner while the payment is "processing".
    virtual std::vector<std::string> processingSteps() const;

    const Money& amount() const { return amount_; }
    bool isPaid() const { return paid_; }
    const std::string& reference() const { return reference_; }

    // OOP: static members -- transaction counter shared by all payment types.
    static int transactionsProcessed() { return processed_; }
    static void resetCounter(int next);

protected:
    // OOP: runtime polymorphism -- the hooks the template method calls.
    virtual void validate() const = 0;        // throw PaymentException if invalid
    virtual void process() {}                 // side effects (e.g. wallet debit)
    virtual std::string referencePrefix() const = 0;

private:
    Money amount_;
    bool paid_;
    std::string reference_;
    static int nextTxn_;
    static int processed_;
};

} // namespace shop
