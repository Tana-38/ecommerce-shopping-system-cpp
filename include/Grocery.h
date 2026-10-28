#pragma once
#include "Product.h"

// Inheritance: Grocery IS-A Product, adding an expiryDate field.
class Grocery : public Product {
private:
    std::string expiryDate;
public:
    Grocery(int id, const std::string& name, double price, int stock, const std::string& expiryDate);

    // Runtime polymorphism: flat Rs 20 off, but never more than the price itself.
    double calculateDiscount() const override;
    std::string getCategory() const override;
    void display() const override;

    std::string getExpiryDate() const;
};
