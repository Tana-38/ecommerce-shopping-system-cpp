#pragma once
#include <memory>
#include <string>
#include <vector>

#include "util/Money.h"

namespace shop {

// OOP: abstraction -- a coupon knows its own rule for turning an order
// subtotal into a discount. Shared limits (minimum order, maximum discount)
// live in the base class; the actual formula is pure virtual.
class Coupon {
public:
    Coupon(const std::string& code, const std::string& description,
           const Money& minOrder, const Money& maxDiscount);
    virtual ~Coupon() = default;

    const std::string& code() const { return code_; }
    const std::string& description() const { return description_; }
    const Money& minOrder() const { return minOrder_; }
    const Money& maxDiscount() const { return maxDiscount_; }   // zero = no cap

    bool isApplicable(const Money& subtotal) const { return subtotal >= minOrder_; }
    // Why the coupon cannot be used right now ("" when it can).
    std::string whyNot(const Money& subtotal) const;

    // Template method: checks the minimum, applies the subclass formula,
    // then clamps to the cap and to the subtotal itself.
    Money discountFor(const Money& subtotal) const;

    // OOP: abstraction -- each subclass supplies its own formula.
    virtual std::string kind() const = 0;

protected:
    virtual Money rawDiscount(const Money& subtotal) const = 0;

private:
    std::string code_;
    std::string description_;
    Money minOrder_;
    Money maxDiscount_;
};

// OOP: inheritance (hierarchical) -- percentage based coupon.
class PercentCoupon : public Coupon {
public:
    PercentCoupon(const std::string& code, const std::string& description, double percent,
                  const Money& minOrder = Money(), const Money& maxDiscount = Money());
    std::string kind() const override { return "Percent"; }
    double percent() const { return percent_; }

protected:
    Money rawDiscount(const Money& subtotal) const override;

private:
    double percent_;
};

// OOP: inheritance (hierarchical) -- flat amount coupon.
class FlatCoupon : public Coupon {
public:
    FlatCoupon(const std::string& code, const std::string& description, const Money& amount,
               const Money& minOrder = Money());
    std::string kind() const override { return "Flat"; }

protected:
    Money rawDiscount(const Money& subtotal) const override;

private:
    Money amount_;
};

// Registry of the coupons the store accepts.
//   SAVE10    -> 10% off, no minimum, discount capped at Rs 1,000
//   FLAT200   -> Rs 200 off on orders of Rs 1,000 or more
//   FESTIVE25 -> 25% off on orders of Rs 5,000 or more, capped at Rs 2,500
class CouponBook {
public:
    // Throws CouponException for an unknown code (case-insensitive lookup).
    static std::shared_ptr<const Coupon> find(const std::string& code);
    static const std::vector<std::shared_ptr<const Coupon>>& all();
};

} // namespace shop
