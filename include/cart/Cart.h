#pragma once
#include <memory>
#include <string>
#include <vector>

#include "cart/CartItem.h"
#include "payment/Coupon.h"
#include "util/Money.h"

namespace shop {

// Store-wide pricing constants (documented in the README / report).
namespace pricing {
const double kGstPercent = 5.0;                 // GST on (subtotal - coupon)
const double kFreeDeliveryFrom = 500.0;         // subtotal at which delivery is free
const double kDeliveryFee = 49.0;               // otherwise
}  // namespace pricing

// Every number shown on the cart / checkout / invoice comes from here.
struct PriceBreakdown {
    Money mrpTotal;          // sum of MRP * qty
    Money productDiscount;   // sum of category discounts
    Money subtotal;          // mrpTotal - productDiscount
    Money couponDiscount;    // 0 if no coupon or not applicable
    Money deliveryFee;       // Rs 49 when subtotal < Rs 500
    Money gst;               // 5% of (subtotal - couponDiscount)
    Money total;             // subtotal - coupon + delivery + gst
    int units = 0;
    std::string couponCode;     // applied coupon (may be empty)
    std::string couponNote;     // why the coupon is not active, if so
    Money totalSavings() const { return productDiscount + couponDiscount; }
};

// OOP: classes & objects -- the shopping cart.
// OOP: STL containers -- items live in a std::vector<CartItem>.
class Cart {
public:
    Cart() = default;

    // OOP: function overloading -- add by product+qty or by a ready CartItem.
    void addItem(const ProductPtr& product, int qty);
    void addItem(const CartItem& item);
    void updateQuantity(int productId, int qty);
    void removeItem(int productId);
    void clear();

    // OOP: operator overloading -- cart += item merges, cart -= id removes.
    Cart& operator+=(const CartItem& item);
    Cart& operator-=(int productId);

    void applyCoupon(const std::string& code);   // throws CouponException
    void removeCoupon();
    bool hasCoupon() const { return static_cast<bool>(coupon_); }
    std::shared_ptr<const Coupon> coupon() const { return coupon_; }

    const std::vector<CartItem>& items() const { return items_; }
    bool isEmpty() const { return items_.empty(); }
    bool contains(int productId) const;
    int quantityOf(int productId) const;
    int units() const;          // total number of units
    std::size_t lineCount() const { return items_.size(); }

    PriceBreakdown breakdown() const;
    Money total() const { return breakdown().total; }

    // OOP: static members -- pure pricing function shared by Cart and Order.
    static PriceBreakdown price(const Money& mrpTotal, const Money& productDiscount, int units,
                                const std::shared_ptr<const Coupon>& coupon);

private:
    std::vector<CartItem> items_;
    std::shared_ptr<const Coupon> coupon_;

    std::vector<CartItem>::iterator find(int productId);
    std::vector<CartItem>::const_iterator find(int productId) const;   // OOP: const overload
};

} // namespace shop
