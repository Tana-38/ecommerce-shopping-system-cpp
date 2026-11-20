#include "core/Grocery.h"
#include <algorithm>
#include <iostream>

namespace shop {

Grocery::Grocery(int id, const std::string& name, double price, int stock, const std::string& expiryDate)
    : Product(id, name, price, stock), expiryDate(expiryDate) {}

double Grocery::calculateDiscount() const {
    return std::min(price, 20.0);
}

std::string Grocery::getCategory() const {
    return "Grocery";
}

std::string Grocery::discountRule() const {
    return price > 20.0 ? "Flat Rs 20 off" : "Flat Rs 20 off (capped at item price)";
}

std::string Grocery::attributeSummary() const {
    return "Best before " + expiryDate;
}

std::vector<std::string> Grocery::attributeFields() const {
    return {expiryDate};
}

std::unique_ptr<Product> Grocery::clone() const {
    return std::unique_ptr<Product>(new Grocery(*this));
}

void Grocery::display() const {
    std::cout << "[" << getCategory() << "] #" << id << " " << name
               << " | Price: Rs " << price
               << " | Discount: Rs " << calculateDiscount()
               << " | Final: Rs " << getFinalPrice()
               << " | Stock: " << stock
               << " | Expiry: " << expiryDate << "\n";
}

std::string Grocery::getExpiryDate() const { return expiryDate; }

} // namespace shop
