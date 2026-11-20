#pragma once
#include <memory>
#include <string>

#include "cart/Cart.h"
#include "core/Customer.h"
#include "payment/Payment.h"
#include "services/Store.h"
#include "ui/Screens.h"

namespace shop {
namespace ui {

// The interactive console application: owns the session (who is logged in,
// their cart) and drives every screen through the Store facade.
class App {
public:
    explicit App(Store& store);
    int run();

private:
    Store& store_;
    std::shared_ptr<Customer> user_;   // nullptr = nobody signed in
    bool admin_;
    Cart cart_;

    // chrome
    std::string sessionInfo() const;
    void present(const std::string& crumb, const std::string& body, const KeyHints& keys) const;

    // input helpers (blank input = cancel, returns false)
    bool askInt(const std::string& label, int lo, int hi, int& out, const std::string& hint = "");
    bool askDouble(const std::string& label, double lo, double hi, double& out,
                   const std::string& hint = "");

    // auth
    bool authMenu();   // false = exit app
    bool loginFlow();
    bool registerFlow();
    bool adminLoginFlow();
    void logout();

    // customer
    void customerHome();
    void browse(const std::string& initialSearch, bool adminMode);
    void productDetail(const ProductPtr& p, bool adminMode);
    void addToCartFlow(const ProductPtr& p);
    void cartScreen();
    void couponFlow();
    void checkout();
    std::unique_ptr<Payment> buildPayment(int method, const Money& total);
    void ordersScreen();
    void orderDetailScreen(const OrderPtr& order, bool adminMode);
    void showInvoice(const Order& order);
    void profileScreen();

    // admin
    void adminHome();
    void adminDashboard();
    void adminAddProduct();
    void adminRestock(const ProductPtr& p);
    void adminPrice(const ProductPtr& p);
    void adminRemove(const ProductPtr& p);
    void adminOrders();
    void adminLowStock();
    void adminCustomers();
};

}  // namespace ui
}  // namespace shop
