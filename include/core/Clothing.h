#pragma once
#include "core/Product.h"

namespace shop {

// Inheritance: Clothing IS-A Product, adding a size field.
// OOP: inheritance (hierarchical) -- second child of Product.
class Clothing : public Product {
private:
    std::string size;
public:
    Clothing(int id, const std::string& name, double price, int stock, const std::string& size);

    // Runtime polymorphism: 20% off if price > Rs 1000, else 5% off.
    double calculateDiscount() const override;
    std::string getCategory() const override;
    std::string discountRule() const override;
    std::string attributeSummary() const override;
    std::vector<std::string> attributeFields() const override;
    std::unique_ptr<Product> clone() const override;
    void display() const override;

    std::string getSize() const;
};

} // namespace shop
