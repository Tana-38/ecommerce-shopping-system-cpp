#include "cart/Cart.h"

#include <algorithm>
#include <numeric>

#include "util/Exceptions.h"

namespace shop {

std::vector<CartItem>::iterator Cart::find(int productId) {
    return std::find_if(items_.begin(), items_.end(),
                        [productId](const CartItem& i) { return i.productId() == productId; });
}

std::vector<CartItem>::const_iterator Cart::find(int productId) const {
    return std::find_if(items_.begin(), items_.end(),
                        [productId](const CartItem& i) { return i.productId() == productId; });
}

void Cart::addItem(const ProductPtr& product, int qty) {
    addItem(CartItem(product, qty));
}

void Cart::addItem(const CartItem& item) {
    // OOP: operator overloading in use -- CartItem::operator== finds the duplicate.
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        it->setQuantity(it->quantity() + item.quantity());   // throws if over stock
    } else {
        items_.push_back(item);
    }
}

void Cart::updateQuantity(int productId, int qty) {
    auto it = find(productId);
    if (it == items_.end()) throw NotFoundException("That product is not in your cart.");
    if (qty == 0) {
        items_.erase(it);
        return;
    }
    it->setQuantity(qty);
}

void Cart::removeItem(int productId) {
    auto it = find(productId);
    if (it == items_.end()) throw NotFoundException("That product is not in your cart.");
    items_.erase(it);
}

void Cart::clear() {
    items_.clear();
    coupon_.reset();
}

Cart& Cart::operator+=(const CartItem& item) {
    addItem(item);
    return *this;
}

Cart& Cart::operator-=(int productId) {
    removeItem(productId);
    return *this;
}

void Cart::applyCoupon(const std::string& code) {
    std::shared_ptr<const Coupon> c = CouponBook::find(code);   // throws if unknown
    PriceBreakdown b = breakdown();
    if (!c->isApplicable(b.subtotal)) throw CouponException(c->whyNot(b.subtotal) + ".");
    coupon_ = c;
}

void Cart::removeCoupon() { coupon_.reset(); }

bool Cart::contains(int productId) const { return find(productId) != items_.end(); }

int Cart::quantityOf(int productId) const {
    auto it = find(productId);
    return it == items_.end() ? 0 : it->quantity();
}

int Cart::units() const {
    // OOP: STL algorithms -- std::accumulate.
    return std::accumulate(items_.begin(), items_.end(), 0,
                           [](int sum, const CartItem& i) { return sum + i.quantity(); });
}

PriceBreakdown Cart::breakdown() const {
    Money mrp = std::accumulate(items_.begin(), items_.end(), Money(),
                                [](const Money& sum, const CartItem& i) { return sum + i.lineMrp(); });
    Money disc = std::accumulate(
        items_.begin(), items_.end(), Money(),
        [](const Money& sum, const CartItem& i) { return sum + i.lineDiscount(); });
    return price(mrp, disc, units(), coupon_);
}

PriceBreakdown Cart::price(const Money& mrpTotal, const Money& productDiscount, int units,
                           const std::shared_ptr<const Coupon>& coupon) {
    PriceBreakdown b;
    b.mrpTotal = mrpTotal;
    b.productDiscount = productDiscount;
    b.subtotal = mrpTotal - productDiscount;
    b.units = units;
    if (coupon) {
        b.couponCode = coupon->code();
        b.couponDiscount = coupon->discountFor(b.subtotal);
        b.couponNote = coupon->whyNot(b.subtotal);
    }
    if (units == 0) return b;   // empty cart: everything stays zero
    b.deliveryFee = b.subtotal < Money(pricing::kFreeDeliveryFrom) ? Money(pricing::kDeliveryFee)
                                                                   : Money();
    b.gst = (b.subtotal - b.couponDiscount).percent(pricing::kGstPercent);
    b.total = b.subtotal - b.couponDiscount + b.deliveryFee + b.gst;
    return b;
}

} // namespace shop
