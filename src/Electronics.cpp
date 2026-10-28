#include "Electronics.h"
#include <iostream>

Electronics::Electronics(int id, const std::string& name, double price, int stock, int warrantyMonths)
    : Product(id, name, price, stock), warrantyMonths(warrantyMonths) {}

double Electronics::calculateDiscount() const {
    return price * 0.10;
}

std::string Electronics::getCategory() const {
    return "Electronics";
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
