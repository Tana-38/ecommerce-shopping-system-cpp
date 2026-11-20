#pragma once
#include <memory>
#include <string>

#include "cart/Cart.h"
#include "core/Customer.h"
#include "services/Store.h"
#include "ui/Screens.h"

namespace shop {
namespace ui {

// Scripted, non-interactive walkthrough of the main features (--demo).
// Runs on the built-in seed data with persistence switched off and a fixed
// clock, so every run looks exactly the same (used for README GIFs).
class Demo {
public:
    explicit Demo(Store& store);
    int run();

private:
    Store& store_;
    std::shared_ptr<Customer> user_;
    Cart cart_;
    int scene_;

    void beat(int ms) const;
    void frame(const std::string& crumb, const std::string& body, bool admin = false) const;
    std::string caption(const std::string& title, const std::string& text) const;
    std::string session(bool admin) const;
    void addQuietly(int productId, int qty);

    void sceneSignIn();
    void sceneBrowse();
    void sceneProduct();
    void sceneCart();
    void sceneCheckout();
    void sceneOrders();
    void sceneAdmin();
    void outro();
};

}  // namespace ui
}  // namespace shop
