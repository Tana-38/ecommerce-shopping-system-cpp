#include "payment/Coupon.h"

#include <algorithm>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

Coupon::Coupon(const std::string& code, const std::string& description,
               const Money& minOrder, const Money& maxDiscount)
    : code_(code), description_(description), minOrder_(minOrder), maxDiscount_(maxDiscount) {}

std::string Coupon::whyNot(const Money& subtotal) const {
    if (isApplicable(subtotal)) return "";
    return "Add " + (minOrder_ - subtotal).str() + " more to use " + code_ +
           " (min order " + minOrder_.shortStr() + ")";
}

Money Coupon::discountFor(const Money& subtotal) const {
    if (!isApplicable(subtotal) || subtotal <= Money()) return Money();
    Money d = rawDiscount(subtotal);   // OOP: runtime polymorphism (virtual call)
    if (!maxDiscount_.isZero() && d > maxDiscount_) d = maxDiscount_;
    if (d > subtotal) d = subtotal;
    return d;
}

PercentCoupon::PercentCoupon(const std::string& code, const std::string& description,
                             double percent, const Money& minOrder, const Money& maxDiscount)
    : Coupon(code, description, minOrder, maxDiscount), percent_(percent) {}

Money PercentCoupon::rawDiscount(const Money& subtotal) const {
    return subtotal.percent(percent_);
}

FlatCoupon::FlatCoupon(const std::string& code, const std::string& description,
                       const Money& amount, const Money& minOrder)
    : Coupon(code, description, minOrder, Money()), amount_(amount) {}

Money FlatCoupon::rawDiscount(const Money& /*subtotal*/) const { return amount_; }

const std::vector<std::shared_ptr<const Coupon>>& CouponBook::all() {
    // OOP: smart pointers -- coupons are shared, immutable objects.
    static const std::vector<std::shared_ptr<const Coupon>> coupons = {
        std::make_shared<PercentCoupon>("SAVE10", "10% off your order (max Rs 1,000)", 10.0,
                                        Money(), Money(1000.0)),
        std::make_shared<FlatCoupon>("FLAT200", "Flat Rs 200 off on orders above Rs 1,000",
                                     Money(200.0), Money(1000.0)),
        std::make_shared<PercentCoupon>("FESTIVE25",
                                        "Festive sale: 25% off on Rs 5,000+ (max Rs 2,500)",
                                        25.0, Money(5000.0), Money(2500.0)),
    };
    return coupons;
}

std::shared_ptr<const Coupon> CouponBook::find(const std::string& code) {
    std::string wanted = util::toUpper(util::trim(code));
    const auto& coupons = all();
    // OOP: STL algorithms -- find_if over the registry.
    auto it = std::find_if(coupons.begin(), coupons.end(),
                           [&](const std::shared_ptr<const Coupon>& c) { return c->code() == wanted; });
    if (it == coupons.end()) {
        throw CouponException("'" + util::trim(code) + "' is not a valid coupon code.");
    }
    return *it;
}

} // namespace shop
