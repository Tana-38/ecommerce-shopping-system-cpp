#include "core/Electronics.h"
#include <iostream>

#include "util/StringUtil.h"

namespace shop {

Electronics::Electronics(int id, const std::string& name, double price, int stock, int warrantyMonths)
    : Product(id, name, price, stock), warrantyMonths(warrantyMonths) {}

double Electronics::calculateDiscount() const {
    return price * 0.10;
}

std::string Electronics::getCategory() const {
    return "Electronics";
}

std::string Electronics::discountRule() const {
    return "10% off on all electronics";
}

std::string Electronics::attributeSummary() const {
    return util::toStr(warrantyMonths) + "-month warranty";
}

std::vector<std::string> Electronics::attributeFields() const {
    return {util::toStr(warrantyMonths)};
}

std::unique_ptr<Product> Electronics::clone() const {
    return std::unique_ptr<Product>(new Electronics(*this));
}

void Electronics::display() const {
    std::cout << "[" << getCategory() << "] #" << id << " " << name
               << " | Price: Rs " << price
               << " | Discount: Rs " << calculateDiscount()
               << " | Final: Rs " << getFinalPrice()
               << " | Stock: " << stock
               << " | Warranty: " << warrantyMonths << " months\n";
}

int Electronics::getWarrantyMonths() const { return warrantyMonths; }

} // namespace shop
