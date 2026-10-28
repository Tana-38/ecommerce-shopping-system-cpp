#include "Product.h"
#include <iostream>

// Encapsulation: all data members live in the base class as protected,
// reachable only through the constructor/getters defined here and in derived classes.
Product::Product(int id, const std::string& name, double price, int stock)
    : id(id), name(name), price(price), stock(stock) {}

// Base implementation of display(); derived classes override this to add
// their own category-specific line while reusing the common fields.
void Product::display() const {
    std::cout << "[" << getCategory() << "] #" << id << " " << name
               << " | Price: Rs " << price
               << " | Discount: Rs " << calculateDiscount()
               << " | Final: Rs " << getFinalPrice()
               << " | Stock: " << stock << "\n";
}

int Product::getId() const { return id; }
std::string Product::getName() const { return name; }
double Product::getPrice() const { return price; }
int Product::getStock() const { return stock; }

bool Product::reduceStock(int qty) {
    if (qty < 0 || qty > stock) return false;
    stock -= qty;
    return true;
}
