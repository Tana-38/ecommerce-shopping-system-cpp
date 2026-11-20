#pragma once
#include <memory>
#include <string>
#include <vector>

#include "core/Product.h"

namespace shop {

// ---- Part A helpers (kept for tests/test_partA.cpp) -------------------------

// Returns a catalog of 9 heap-allocated products (3 per category), owned by
// the caller. Each slot is a Product* (base pointer) even though the actual
// object is an Electronics/Clothing/Grocery -- this is what lets Part B's
// Cart/Order work with any product category uniformly.
std::vector<Product*> createSampleCatalog();

// Deletes every product in the catalog. Because ~Product() is virtual,
// `delete p` here correctly runs the derived class's destructor too.
void freeCatalog(std::vector<Product*>& catalog);

// ---- Product factory --------------------------------------------------------

// Builds the right subclass from a type name ("Electronics", "Smartphone",
// "Clothing", "Grocery") plus its category-specific attribute fields.
// Throws InvalidInputException for unknown types or bad attributes.
std::unique_ptr<Product> createProduct(const std::string& type, int id,
                                       const std::string& name, double price, int stock,
                                       const std::vector<std::string>& attributes);

// ---- Catalog query view -----------------------------------------------------

enum class SortKey { Featured, PriceLowHigh, PriceHighLow, Name, BiggestDiscount, Rating };

// A read-only, chainable view over shared products:
//   catalog.inCategory("Clothing").matching("shirt").sortedBy(SortKey::PriceLowHigh)
// OOP: classes & objects, const correctness (every query is a const member).
class Catalog {
public:
    Catalog() = default;
    explicit Catalog(std::vector<ProductPtr> items);

    Catalog inCategory(const std::string& category) const;   // "All" keeps everything
    Catalog matching(const std::string& term) const;          // name contains term
    Catalog inStockOnly() const;
    Catalog sortedBy(SortKey key) const;

    const std::vector<ProductPtr>& items() const { return items_; }
    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    static std::vector<std::string> categories();   // All, Electronics, Clothing, Grocery
    static std::string sortLabel(SortKey key);
    static SortKey nextSort(SortKey key);

private:
    std::vector<ProductPtr> items_;
};

} // namespace shop
