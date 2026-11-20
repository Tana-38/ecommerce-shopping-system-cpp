#include "payment/Payment.h"

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

int Payment::nextTxn_ = 482101;
int Payment::processed_ = 0;

Payment::Payment(const Money& amount) : amount_(amount), paid_(false) {}

Payment::~Payment() {}

std::string Payment::pay() {
    if (paid_) throw PaymentException("This payment has already been completed.");
    if (amount_ <= Money()) throw PaymentException("Payment amount must be positive.");
    validate();   // OOP: runtime polymorphism -- subclass validation
    process();    // OOP: runtime polymorphism -- subclass side effects
    reference_ = referencePrefix() + "-" + util::toStr(nextTxn_++);
    paid_ = true;
    ++processed_;
    return reference_;
}

std::vector<std::string> Payment::processingSteps() const {
    return {"Connecting to payment gateway", "Authorising " + amount_.str(),
            "Confirming your order"};
}

void Payment::resetCounter(int next) { nextTxn_ = next; }

} // namespace shop
