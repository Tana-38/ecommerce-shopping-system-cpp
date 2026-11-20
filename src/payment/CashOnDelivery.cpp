#include "payment/CashOnDelivery.h"

#include "util/Exceptions.h"

namespace shop {

CashOnDelivery::CashOnDelivery(const Money& amount) : Payment(amount) {}

void CashOnDelivery::validate() const {
    if (amount() > limit()) {
        throw PaymentException("Cash on Delivery is available only for orders up to " +
                               limit().str() + ".");
    }
}

std::vector<std::string> CashOnDelivery::processingSteps() const {
    return {"Checking COD availability for your pincode", "Reserving your items",
            "Order confirmed - pay " + amount().str() + " on delivery"};
}

} // namespace shop
