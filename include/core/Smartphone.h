#pragma once
#include "core/Electronics.h"

namespace shop {

// OOP: inheritance (multilevel) -- Product -> Electronics -> Smartphone.
// A Smartphone is still in the "Electronics" category but adds storage and
// stacks a 5% launch offer on top of the regular Electronics discount.
class Smartphone : public Electronics {
private:
    int storageGb;
public:
    Smartphone(int id, const std::string& name, double price, int stock,
               int warrantyMonths, int storageGb);

    // OOP: runtime polymorphism -- overrides a function Electronics already
    // overrode, and re-uses the parent version via Electronics::.
    double calculateDiscount() const override;   // 10% + 5% = 15% of price
    std::string getType() const override;
    std::string discountRule() const override;
    std::string attributeSummary() const override;
    std::vector<std::string> attributeFields() const override;
    std::unique_ptr<Product> clone() const override;

    int getStorageGb() const;
};

} // namespace shop
