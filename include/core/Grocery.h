#pragma once
#include "core/Product.h"

namespace shop {

// Inheritance: Grocery IS-A Product, adding an expiryDate field.
// OOP: inheritance (hierarchical) -- third child of Product.
class Grocery : public Product {
private:
    std::string expiryDate;
public:
    Grocery(int id, const std::string& name, double price, int stock, const std::string& expiryDate);

    // Runtime polymorphism: flat Rs 20 off, but never more than the price itself.
    double calculateDiscount() const override;
    std::string getCategory() const override;
    std::string discountRule() const override;
    std::string attributeSummary() const override;
    std::vector<std::string> attributeFields() const override;
    std::unique_ptr<Product> clone() const override;
    void display() const override;

    std::string getExpiryDate() const;
};

} // namespace shop
