#pragma once
#include "Product.h"

// Inheritance: Electronics IS-A Product, adding a field (warrantyMonths)
// that only makes sense for this category.
class Electronics : public Product {
private:
    int warrantyMonths;
public:
    Electronics(int id, const std::string& name, double price, int stock, int warrantyMonths);

    // Runtime polymorphism: these overrides are picked at runtime via the
    // vtable whenever called through a Product* / Product&.
    double calculateDiscount() const override;   // 10% of price
    std::string getCategory() const override;
    void display() const override;

    int getWarrantyMonths() const;
};
