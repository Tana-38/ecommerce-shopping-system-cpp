#include "core/Catalog.h"

#include <algorithm>
#include <iterator>

#include "core/Clothing.h"
#include "core/Electronics.h"
#include "core/Grocery.h"
#include "core/Smartphone.h"
#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

std::vector<Product*> createSampleCatalog() {
    std::vector<Product*> catalog;

    // Electronics: id, name, price (Rs), stock, warrantyMonths
    catalog.push_back(new Electronics(1, "Bluetooth Earbuds", 1999.0, 25, 6));
    catalog.push_back(new Electronics(2, "Smartphone", 18999.0, 10, 12));
    catalog.push_back(new Electronics(3, "Laptop", 52999.0, 5, 24));

    // Clothing: id, name, price (Rs), stock, size
    catalog.push_back(new Clothing(4, "Cotton T-Shirt", 499.0, 50, "M"));
    catalog.push_back(new Clothing(5, "Denim Jacket", 1899.0, 20, "L"));
    catalog.push_back(new Clothing(6, "Formal Shirt", 1199.0, 30, "XL"));

    // Grocery: id, name, price (Rs), stock, expiryDate
    catalog.push_back(new Grocery(7, "Basmati Rice 5kg", 450.0, 100, "2026-06-30"));
    catalog.push_back(new Grocery(8, "Sunflower Oil 1L", 180.0, 80, "2026-03-15"));
    catalog.push_back(new Grocery(9, "Toor Dal 1kg", 15.0, 60, "2026-04-20")); // price < discount

    return catalog;
}

void freeCatalog(std::vector<Product*>& catalog) {
    for (Product* p : catalog) {
        delete p;   // virtual ~Product() ensures the correct derived destructor runs
    }
    catalog.clear();
}

namespace {
int attrInt(const std::vector<std::string>& attrs, std::size_t index, const std::string& what) {
    int value = 0;
    if (index >= attrs.size() || !util::parseInt(attrs[index], value) || value < 0) {
        throw InvalidInputException(what + " must be a non-negative whole number.");
    }
    return value;
}

std::string attrText(const std::vector<std::string>& attrs, std::size_t index,
                     const std::string& what) {
    if (index >= attrs.size() || util::trim(attrs[index]).empty()) {
        throw InvalidInputException(what + " is required.");
    }
    return util::trim(attrs[index]);
}
}  // namespace

std::unique_ptr<Product> createProduct(const std::string& type, int id,
                                       const std::string& name, double price, int stock,
                                       const std::vector<std::string>& attributes) {
    if (util::trim(name).size() < 2) throw InvalidInputException("Product name is too short.");
    if (!(price > 0.0)) throw InvalidInputException("Price must be positive.");
    if (stock < 0) throw InvalidInputException("Stock cannot be negative.");

    std::string t = util::toLower(util::trim(type));
    std::unique_ptr<Product> p;
    if (t == "electronics") {
        p.reset(new Electronics(id, name, price, stock, attrInt(attributes, 0, "Warranty")));
    } else if (t == "smartphone") {
        p.reset(new Smartphone(id, name, price, stock, attrInt(attributes, 0, "Warranty"),
                               attrInt(attributes, 1, "Storage")));
    } else if (t == "clothing") {
        p.reset(new Clothing(id, name, price, stock, attrText(attributes, 0, "Size")));
    } else if (t == "grocery") {
        p.reset(new Grocery(id, name, price, stock, attrText(attributes, 0, "Expiry date")));
    } else {
        throw InvalidInputException("Unknown product type: " + type);
    }
    return p;
}

Catalog::Catalog(std::vector<ProductPtr> items) : items_(std::move(items)) {}

Catalog Catalog::inCategory(const std::string& category) const {
    if (category.empty() || category == "All") return *this;
    std::vector<ProductPtr> out;
    // OOP: STL algorithms -- copy_if with a lambda predicate.
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(out),
                 [&](const ProductPtr& p) { return p->getCategory() == category; });
    return Catalog(out);
}

Catalog Catalog::matching(const std::string& term) const {
    std::string t = util::trim(term);
    if (t.empty()) return *this;
    std::vector<ProductPtr> out;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(out),
                 [&](const ProductPtr& p) {
                     return util::containsIgnoreCase(p->getName(), t) ||
                            util::containsIgnoreCase(p->getType(), t);
                 });
    return Catalog(out);
}

Catalog Catalog::inStockOnly() const {
    std::vector<ProductPtr> out;
    std::copy_if(items_.begin(), items_.end(), std::back_inserter(out),
                 [](const ProductPtr& p) { return p->getStock() > 0; });
    return Catalog(out);
}

Catalog Catalog::sortedBy(SortKey key) const {
    std::vector<ProductPtr> out = items_;
    // OOP: STL algorithms -- stable_sort with comparator lambdas.
    switch (key) {
        case SortKey::PriceLowHigh:
            std::stable_sort(out.begin(), out.end(), [](const ProductPtr& a, const ProductPtr& b) {
                return a->getFinalPrice() < b->getFinalPrice();
            });
            break;
        case SortKey::PriceHighLow:
            std::stable_sort(out.begin(), out.end(), [](const ProductPtr& a, const ProductPtr& b) {
                return a->getFinalPrice() > b->getFinalPrice();
            });
            break;
        case SortKey::Name:
            std::stable_sort(out.begin(), out.end(), [](const ProductPtr& a, const ProductPtr& b) {
                return util::toLower(a->getName()) < util::toLower(b->getName());
            });
            break;
        case SortKey::BiggestDiscount:
            std::stable_sort(out.begin(), out.end(), [](const ProductPtr& a, const ProductPtr& b) {
                return a->discountPercent() > b->discountPercent();
            });
            break;
        case SortKey::Rating:
            std::stable_sort(out.begin(), out.end(), [](const ProductPtr& a, const ProductPtr& b) {
                return a->getRating() > b->getRating();
            });
            break;
        case SortKey::Featured:
        default:
            std::stable_sort(out.begin(), out.end(), [](const ProductPtr& a, const ProductPtr& b) {
                return a->getId() < b->getId();
            });
            break;
    }
    return Catalog(out);
}

std::vector<std::string> Catalog::categories() {
    return {"All", "Electronics", "Clothing", "Grocery"};
}

std::string Catalog::sortLabel(SortKey key) {
    switch (key) {
        case SortKey::PriceLowHigh: return "Price: low to high";
        case SortKey::PriceHighLow: return "Price: high to low";
        case SortKey::Name: return "Name A-Z";
        case SortKey::BiggestDiscount: return "Biggest discount";
        case SortKey::Rating: return "Top rated";
        case SortKey::Featured:
        default: return "Featured";
    }
}

SortKey Catalog::nextSort(SortKey key) {
    switch (key) {
        case SortKey::Featured: return SortKey::PriceLowHigh;
        case SortKey::PriceLowHigh: return SortKey::PriceHighLow;
        case SortKey::PriceHighLow: return SortKey::BiggestDiscount;
        case SortKey::BiggestDiscount: return SortKey::Rating;
        case SortKey::Rating: return SortKey::Name;
        case SortKey::Name:
        default: return SortKey::Featured;
    }
}

} // namespace shop
