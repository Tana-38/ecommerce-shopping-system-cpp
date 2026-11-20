#include "cart/CartItem.h"

#include <ostream>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

CartItem::CartItem(const ProductPtr& product, int quantity) : product_(product), quantity_(0) {
    if (!product_) throw InvalidInputException("Cart item needs a product.");
    setQuantity(quantity);
}

// OOP: copy constructor -- copies the shared_ptr (shared ownership, ref count +1).
CartItem::CartItem(const CartItem& other) : product_(other.product_), quantity_(other.quantity_) {}

void CartItem::setQuantity(int qty) {
    if (qty <= 0) throw InvalidInputException("Quantity must be at least 1.");
    if (qty > product_->getStock()) {
        throw OutOfStockException(product_->getName(), qty, product_->getStock());
    }
    quantity_ = qty;
}

Money CartItem::unitPrice() const { return Money(product_->getPrice()); }
Money CartItem::unitDiscount() const { return Money(product_->calculateDiscount()); }
Money CartItem::unitFinal() const { return unitPrice() - unitDiscount(); }
Money CartItem::lineMrp() const { return unitPrice() * quantity_; }
Money CartItem::lineDiscount() const { return unitDiscount() * quantity_; }
Money CartItem::lineTotal() const { return unitFinal() * quantity_; }

bool CartItem::operator==(const CartItem& other) const {
    return productId() == other.productId();
}

std::ostream& operator<<(std::ostream& os, const CartItem& item) {
    return os << item.product_->getName() << " x" << item.quantity_ << " = "
              << item.lineTotal().plain();
}

} // namespace shop
