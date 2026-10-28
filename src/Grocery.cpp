#include "Grocery.h"
#include <algorithm>
#include <iostream>

Grocery::Grocery(int id, const std::string& name, double price, int stock, const std::string& expiryDate)
    : Product(id, name, price, stock), expiryDate(expiryDate) {}

double Grocery::calculateDiscount() const {
    return std::min(price, 20.0);
}

std::string Grocery::getCategory() const {
    return "Grocery";
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
