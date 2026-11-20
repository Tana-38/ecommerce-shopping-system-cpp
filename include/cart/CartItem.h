#pragma once
#include <iosfwd>
#include <memory>

#include "core/Product.h"
#include "util/Money.h"

namespace shop {

// One line in the cart: a shared product + a quantity.
// OOP: classes & objects, encapsulation, operator overloading (== and <<).
class CartItem {
public:
    // OOP: constructors -- parameterised + explicit copy constructor.
    CartItem(const ProductPtr& product, int quantity);
    CartItem(const CartItem& other);
    CartItem& operator=(const CartItem& other) = default;
    ~CartItem() = default;

    const ProductPtr& product() const { return product_; }
    int productId() const { return product_->getId(); }
    int quantity() const { return quantity_; }
    void setQuantity(int qty);   // OOP: encapsulation -- validated (1..stock)

    Money unitPrice() const;      // MRP per unit
    Money unitDiscount() const;   // category discount per unit
    Money unitFinal() const;      // price after category discount
    Money lineMrp() const;        // unitPrice * qty
    Money lineDiscount() const;
    Money lineTotal() const;      // unitFinal * qty

    // OOP: operator overloading -- two items are "equal" when they refer to
    // the same product, which is how the cart merges duplicates.
    bool operator==(const CartItem& other) const;
    bool operator!=(const CartItem& other) const { return !(*this == other); }

    // OOP: friend function.
    friend std::ostream& operator<<(std::ostream& os, const CartItem& item);

private:
    ProductPtr product_;
    int quantity_;
};

} // namespace shop
