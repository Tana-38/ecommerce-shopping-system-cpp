#pragma once
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "cart/Cart.h"
#include "util/Money.h"

namespace shop {

class Customer;

enum class OrderStatus { Placed, Shipped, Delivered, Cancelled };

std::string statusName(OrderStatus s);
OrderStatus statusFromName(const std::string& s);

// Snapshot of one purchased product. Prices are frozen at checkout time so
// later price changes in the catalog never rewrite order history.
struct OrderLine {
    int productId = 0;
    std::string name;
    std::string category;
    Money unitPrice;
    Money unitDiscount;
    int quantity = 0;

    Money unitFinal() const { return unitPrice - unitDiscount; }
    Money lineTotal() const { return unitFinal() * quantity; }
};

// OOP: classes & objects -- an immutable record of a completed checkout.
class Order {
public:
    // OOP: constructors -- default and parameterised.
    Order();
    Order(int id, int customerId, const std::string& customerName, const std::string& date,
          const std::string& address, const std::vector<OrderLine>& lines,
          const std::string& couponCode);

    // Builds an order from the cart contents (does not touch stock).
    static std::shared_ptr<Order> fromCart(const Cart& cart, const Customer& customer,
                                           const std::string& address, const std::string& date);

    int getId() const { return id_; }
    int getCustomerId() const { return customerId_; }
    const std::string& getCustomerName() const { return customerName_; }
    const std::string& getDate() const { return date_; }
    const std::string& getAddress() const { return address_; }
    const std::vector<OrderLine>& getLines() const { return lines_; }
    const std::string& getCouponCode() const { return couponCode_; }
    const std::string& getPaymentMethod() const { return paymentMethod_; }
    const std::string& getPaymentRef() const { return paymentRef_; }
    OrderStatus getStatus() const { return status_; }

    // Totals are recomputed from the frozen lines with the same pricing rules
    // as the cart, so an order and its invoice always agree.
    PriceBreakdown totals() const;
    Money total() const { return totals().total; }
    int units() const;

    void setPayment(const std::string& method, const std::string& reference);
    void setStatus(OrderStatus s) { status_ = s; }
    bool isCancellable() const;
    void cancel();   // throws InvalidInputException when not cancellable

    // OOP: static members -- order number generator + count of orders placed.
    static int generateId();
    static void observeId(int usedId);
    static int placedThisSession() { return placedThisSession_; }
    static void notePlaced() { ++placedThisSession_; }

    // OOP: friend function -- operator<< prints a one-line summary.
    friend std::ostream& operator<<(std::ostream& os, const Order& o);

private:
    int id_;
    int customerId_;
    std::string customerName_;
    std::string date_;
    std::string address_;
    std::vector<OrderLine> lines_;
    std::string couponCode_;
    std::string paymentMethod_;
    std::string paymentRef_;
    OrderStatus status_;

    static int nextId_;
    static int placedThisSession_;
};

using OrderPtr = std::shared_ptr<Order>;

} // namespace shop
