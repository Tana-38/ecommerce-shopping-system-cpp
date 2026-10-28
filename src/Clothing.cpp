#include "Clothing.h"
#include <iostream>

Clothing::Clothing(int id, const std::string& name, double price, int stock, const std::string& size)
    : Product(id, name, price, stock), size(size) {}

double Clothing::calculateDiscount() const {
    return price > 1000.0 ? price * 0.20 : price * 0.05;
}

std::string Clothing::getCategory() const {
    return "Clothing";
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
