#include "cart/Order.h"

#include <numeric>
#include <ostream>

#include "core/Customer.h"
#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

int Order::nextId_ = 1001;
int Order::placedThisSession_ = 0;

std::string statusName(OrderStatus s) {
    switch (s) {
        case OrderStatus::Placed: return "Placed";
        case OrderStatus::Shipped: return "Shipped";
        case OrderStatus::Delivered: return "Delivered";
        case OrderStatus::Cancelled: return "Cancelled";
    }
    return "Placed";
}

OrderStatus statusFromName(const std::string& s) {
    if (s == "Shipped") return OrderStatus::Shipped;
    if (s == "Delivered") return OrderStatus::Delivered;
    if (s == "Cancelled") return OrderStatus::Cancelled;
    return OrderStatus::Placed;
}

Order::Order() : id_(0), customerId_(0), status_(OrderStatus::Placed) {}

Order::Order(int id, int customerId, const std::string& customerName, const std::string& date,
             const std::string& address, const std::vector<OrderLine>& lines,
             const std::string& couponCode)
    : id_(id), customerId_(customerId), customerName_(customerName), date_(date),
      address_(address), lines_(lines), couponCode_(couponCode), status_(OrderStatus::Placed) {
    observeId(id);
}

std::shared_ptr<Order> Order::fromCart(const Cart& cart, const Customer& customer,
                                       const std::string& address, const std::string& date) {
    if (cart.isEmpty()) throw InvalidInputException("Your cart is empty.");
    std::vector<OrderLine> lines;
    for (const CartItem& item : cart.items()) {
        OrderLine line;
        line.productId = item.productId();
        line.name = item.product()->getName();
        line.category = item.product()->getCategory();
        line.unitPrice = item.unitPrice();
        line.unitDiscount = item.unitDiscount();
        line.quantity = item.quantity();
        lines.push_back(line);
    }
    // Only keep the coupon code if it actually applies to this cart.
    PriceBreakdown b = cart.breakdown();
    std::string code = b.couponDiscount.isZero() ? "" : b.couponCode;
    return std::make_shared<Order>(generateId(), customer.getId(), customer.getName(), date,
                                   address, lines, code);
}

PriceBreakdown Order::totals() const {
    Money mrp;
    Money disc;
    for (const OrderLine& l : lines_) {
        mrp += l.unitPrice * l.quantity;
        disc += l.unitDiscount * l.quantity;
    }
    std::shared_ptr<const Coupon> coupon;
    if (!couponCode_.empty()) {
        try {
            coupon = CouponBook::find(couponCode_);
        } catch (const CouponException&) {
            coupon.reset();   // retired coupon code: price without it
        }
    }
    return Cart::price(mrp, disc, units(), coupon);
}

int Order::units() const {
    return std::accumulate(lines_.begin(), lines_.end(), 0,
                           [](int s, const OrderLine& l) { return s + l.quantity; });
}

void Order::setPayment(const std::string& method, const std::string& reference) {
    paymentMethod_ = method;
    paymentRef_ = reference;
}

bool Order::isCancellable() const {
    return status_ == OrderStatus::Placed || status_ == OrderStatus::Shipped;
}

void Order::cancel() {
    if (!isCancellable()) {
        throw InvalidInputException("Order #" + util::toStr(id_) +
                                    " cannot be cancelled once it is " +
                                    util::toLower(statusName(status_)) + ".");
    }
    status_ = OrderStatus::Cancelled;
}

int Order::generateId() { return nextId_++; }

void Order::observeId(int usedId) {
    if (usedId >= nextId_) nextId_ = usedId + 1;
}

std::ostream& operator<<(std::ostream& os, const Order& o) {
    return os << "Order #" << o.id_ << " | " << o.date_ << " | " << o.customerName_ << " | "
              << o.lines_.size() << " line(s) | " << o.total().plain() << " | "
              << statusName(o.status_);
}

} // namespace shop
