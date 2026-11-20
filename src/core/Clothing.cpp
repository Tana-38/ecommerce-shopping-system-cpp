#include "core/Clothing.h"
#include <iostream>

namespace shop {

Clothing::Clothing(int id, const std::string& name, double price, int stock, const std::string& size)
    : Product(id, name, price, stock), size(size) {}

double Clothing::calculateDiscount() const {
    return price > 1000.0 ? price * 0.20 : price * 0.05;
}

std::string Clothing::getCategory() const {
    return "Clothing";
}

std::string Clothing::discountRule() const {
    return price > 1000.0 ? "20% off (items above Rs 1,000)" : "5% off (items up to Rs 1,000)";
}

std::string Clothing::attributeSummary() const {
    return "Size " + size;
}

std::vector<std::string> Clothing::attributeFields() const {
    return {size};
}

std::unique_ptr<Product> Clothing::clone() const {
    return std::unique_ptr<Product>(new Clothing(*this));
}

void Clothing::display() const {
    std::cout << "[" << getCategory() << "] #" << id << " " << name
               << " | Price: Rs " << price
               << " | Discount: Rs " << calculateDiscount()
               << " | Final: Rs " << getFinalPrice()
               << " | Stock: " << stock
               << " | Size: " << size << "\n";
}

std::string Clothing::getSize() const { return size; }

} // namespace shop
