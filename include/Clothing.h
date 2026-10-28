#pragma once
#include "Product.h"

// Inheritance: Clothing IS-A Product, adding a size field.
class Clothing : public Product {
private:
    std::string size;
public:
    Clothing(int id, const std::string& name, double price, int stock, const std::string& size);

    // Runtime polymorphism: 20% off if price > Rs 1000, else 5% off.
    double calculateDiscount() const override;
    std::string getCategory() const override;
    void display() const override;

    std::string getSize() const;
};
