#include "core/Smartphone.h"

#include "util/StringUtil.h"

namespace shop {

// OOP: constructors -- chained through the inheritance chain
// (Smartphone -> Electronics -> Product).
Smartphone::Smartphone(int id, const std::string& name, double price, int stock,
                       int warrantyMonths, int storageGb)
    : Electronics(id, name, price, stock, warrantyMonths), storageGb(storageGb) {}

double Smartphone::calculateDiscount() const {
    // Parent rule (10%) plus an extra 5% launch offer.
    return Electronics::calculateDiscount() + price * 0.05;
}

std::string Smartphone::getType() const { return "Smartphone"; }

std::string Smartphone::discountRule() const {
    return "10% electronics offer + 5% smartphone launch offer";
}

std::string Smartphone::attributeSummary() const {
    return util::toStr(storageGb) + " GB | " + Electronics::attributeSummary();
}

std::vector<std::string> Smartphone::attributeFields() const {
    return {util::toStr(getWarrantyMonths()), util::toStr(storageGb)};
}

std::unique_ptr<Product> Smartphone::clone() const {
    return std::unique_ptr<Product>(new Smartphone(*this));
}

int Smartphone::getStorageGb() const { return storageGb; }

} // namespace shop
