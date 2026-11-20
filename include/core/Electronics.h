#pragma once
#include "core/Product.h"

namespace shop {

// Inheritance: Electronics IS-A Product, adding a field (warrantyMonths)
// that only makes sense for this category.
// OOP: inheritance (hierarchical) -- Product -> Electronics / Clothing / Grocery.
class Electronics : public Product {
private:
    int warrantyMonths;
public:
    Electronics(int id, const std::string& name, double price, int stock, int warrantyMonths);

    // Runtime polymorphism: these overrides are picked at runtime via the
    // vtable whenever called through a Product* / Product&.
    // OOP: runtime polymorphism -- overrides of the pure virtual functions.
    double calculateDiscount() const override;   // 10% of price
    std::string getCategory() const override;
    std::string discountRule() const override;
    std::string attributeSummary() const override;
    std::vector<std::string> attributeFields() const override;
    std::unique_ptr<Product> clone() const override;
    void display() const override;

    int getWarrantyMonths() const;
};

} // namespace shop
