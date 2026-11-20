#include "ui/Demo.h"

#include "payment/CardPayment.h"
#include "payment/UpiPayment.h"
#include "ui/Animation.h"
#include "ui/Color.h"
#include "ui/Menu.h"
#include "ui/Table.h"
#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {

using util::toStr;

namespace {
const int kScenes = 7;
const RGB kWhite{255, 255, 255};
}  // namespace

Demo::Demo(Store& store) : store_(store), scene_(0) {}

void Demo::beat(int ms) const { Terminal::sleepMs(ms); }

std::string Demo::session(bool admin) const {
    if (admin) return style::paint("● ", theme::kGold) + "admin  ·  Administrator";
    std::string who = user_ ? user_->firstName() : "Guest";
    std::string cart = "Cart " + toStr(cart_.units());
    if (!cart_.isEmpty()) cart += " · " + cart_.total().shortStr();
    return style::paint("● ", user_ ? theme::kSuccess : theme::kMuted) + who +
           style::paint("  │  ", theme::kFaint) + cart;
}

std::string Demo::caption(const std::string& title, const std::string& text) const {
    std::string tag = style::badge("DEMO " + toStr(scene_) + "/" + toStr(kScenes), theme::kAccent, kWhite);
    return "  " + tag + "  " + style::paintBold(title, kWhite) + style::muted("  —  " + text) + "\n\n";
}

void Demo::frame(const std::string& crumb, const std::string& body, bool admin) const {
    Terminal::clear();
    Terminal::write(screens::header(crumb, session(admin)) + "\n" + body);
    Terminal::flush();
}

void Demo::addQuietly(int productId, int qty) {
    store_.addToCart(cart_, productId, qty);
    ProductPtr p = store_.product(productId);
    anim::spinner("Adding " + toStr(qty) + " x " + p->getName(), 380);
}

int Demo::run() {
    Money::setSymbol("₹");
    store_.setPersistence(false);        // never touch data/*.csv in demo mode
    util::setFixedNow("2026-11-20 10:30");
    Payment::resetCounter(482109);

    screens::splash(true);
    beat(900);
    sceneSignIn();
    sceneBrowse();
    sceneProduct();
    sceneCart();
    sceneCheckout();
    sceneOrders();
    sceneAdmin();
    outro();
    util::clearFixedNow();
    return 0;
}

void Demo::sceneSignIn() {
    ++scene_;
    std::string cap = caption("Sign in", "customers log in with email + password");
    frame("Welcome", cap);
    Terminal::write(indentLines(screens::logoLines(0.1), 2 + (kPageWidth - 81) / 2) + "\n");
    Menu menu("Get started", {MenuItem("Log in", "existing customer"),
                              MenuItem("Create an account", "₹500 welcome bonus"),
                              MenuItem("Continue as guest", "browse & build a cart"),
                              MenuItem("Admin console", "staff only"), MenuItem("Exit", "")});
    menu.animateTo(0);
    beat(500);

    frame("Log in", cap);
    screens::sectionTitle("Welcome back", "attempt 1 of 3");
    screens::fakeInput("Email", "tanaya@shop.com");
    screens::fakeInput("Password", "pass123", true);
    Terminal::write("\n");
    user_ = store_.auth().login("tanaya@shop.com", "pass123");
    anim::spinner("Signing you in securely", 800);
    screens::notify(Tone::Success, user_->greeting() + "! Wallet balance " + user_->getWallet().str());
    beat(1300);
}

void Demo::sceneBrowse() {
    ++scene_;
    std::string cap = caption("Browse, filter & sort", "23 products, three categories, live discounts");
    frame("Home", cap);
    screens::sectionTitle("Hello, " + user_->firstName(), "what would you like to do today?");
    Menu home("Main menu", {MenuItem("Browse catalog", "23 products"), MenuItem("Search products", ""),
                            MenuItem("My cart", "empty"), MenuItem("Checkout", "", false),
                            MenuItem("My orders", "2 orders"), MenuItem("Profile & wallet", ""),
                            MenuItem("Log out", "")});
    home.animateTo(0);

    std::vector<ProductPtr> all = store_.catalog().sortedBy(SortKey::Featured).items();
    for (int sel = 0; sel <= 3; ++sel) {
        frame("Catalog", cap + screens::filterBar("All", SortKey::Featured, "", all.size()) + "\n" +
                             screens::productTable(all, sel, 0, 12));
        beat(sel == 0 ? 1800 : 260);
    }
    beat(600);

    std::vector<ProductPtr> elec = store_.catalog().inCategory("Electronics").sortedBy(SortKey::Featured).items();
    frame("Catalog", cap + screens::filterBar("Electronics", SortKey::Featured, "", elec.size()) + "\n" +
                         screens::productTable(elec, 0, 0, 12));
    beat(1400);
    std::vector<ProductPtr> byDeal =
        store_.catalog().inCategory("Electronics").sortedBy(SortKey::BiggestDiscount).items();
    frame("Catalog", cap + screens::filterBar("Electronics", SortKey::BiggestDiscount, "", byDeal.size()) +
                         "\n" + screens::productTable(byDeal, 0, 0, 12));
    beat(1800);

    std::vector<ProductPtr> found = store_.catalog().matching("shirt").sortedBy(SortKey::PriceLowHigh).items();
    frame("Catalog", cap + screens::filterBar("All", SortKey::PriceLowHigh, "shirt", found.size()) + "\n" +
                         screens::productTable(found, 0, 0, 12));
    beat(1600);
}

void Demo::sceneProduct() {
    ++scene_;
    std::string cap = caption("Product details", "polymorphic discount rules per category");
    ProductPtr phone = store_.product(2);
    frame("Catalog › Product", cap + screens::productCard(*phone, 0) + "\n");
    Menu actions("Actions", {MenuItem("Add to cart", "choose quantity"),
                             MenuItem("Buy now", "add 1 & go to cart"), MenuItem("Back", "")}, 40);
    actions.animateTo(0);
    screens::fakeInput("Quantity", "1");
    store_.addToCart(cart_, 2, 1);
    anim::spinner("Adding to your cart", 450);
    screens::notify(Tone::Success, "1 x " + phone->getName() + " added. Cart total " + cart_.total().str());
    beat(1100);

    frame("Catalog", cap);
    screens::sectionTitle("Filling the cart", "saree, rice and butter");
    addQuietly(15, 1);
    addQuietly(17, 2);
    addQuietly(20, 2);
    screens::notify(Tone::Success, toStr(cart_.units()) + " items in cart · " + cart_.total().str());
    beat(1100);
}

void Demo::sceneCart() {
    ++scene_;
    std::string cap = caption("Cart & coupons", "operator+= / -= on Cart, coupon rules, live totals");
    frame("Cart", cap + screens::cartTable(cart_, 0) + "\n" + screens::cartSummary(cart_));
    beat(2200);

    Terminal::write("\n");
    screens::fakeInput("Coupon code", "MEGA50");
    try {
        cart_.applyCoupon("MEGA50");
    } catch (const CouponException& e) {   // demonstrates the custom exception hierarchy
        screens::showError(e);
    }
    beat(1200);
    screens::fakeInput("Coupon code", "FESTIVE25");
    cart_.applyCoupon("FESTIVE25");
    anim::spinner("Applying FESTIVE25", 500);
    screens::notify(Tone::Success, "Coupon applied - you save " + cart_.breakdown().couponDiscount.str() + " more!");
    beat(1000);
    frame("Cart", cap + screens::cartTable(cart_, -1) + "\n" + screens::cartSummary(cart_));
    beat(2400);
}

void Demo::sceneCheckout() {
    ++scene_;
    std::string cap = caption("Checkout & payment", "Payment -> UPI / Card / COD / Wallet via base pointer");
    PriceBreakdown b = cart_.breakdown();
    std::string summary = indentLines(boxLines(screens::breakdownLines(b, 40), 40, "Order summary",
                                               theme::kSecondary), 2);
    frame("Checkout", cap + screens::stepper(0) + summary + "\n");
    Menu addr("Deliver to", {MenuItem(user_->getAddress(), "saved"), MenuItem("A different address", ""),
                             MenuItem("Back to cart", "")}, 56);
    addr.animateTo(0);

    Menu pay("Pay with", {MenuItem("UPI", "GPay / PhonePe / Paytm"),
                          MenuItem("Credit / Debit card", "Visa, Mastercard, RuPay"),
                          MenuItem("Cash on Delivery", "up to ₹50,000"),
                          MenuItem("ShopSphere Wallet", "balance " + user_->getWallet().shortStr()),
                          MenuItem("Back to cart", "")}, 56);
    std::string payHead = cap + screens::stepper(1) + "  " + style::muted("Amount payable  ") +
                          style::paintBold(b.total.str(), theme::kSecondary) +
                          style::muted("   ·   delivering to " + user_->getAddress()) + "\n\n";

    // Attempt 1: a card the bank declines -> PaymentException -> failure animation.
    frame("Checkout › Payment", payHead);
    pay.animateTo(1);
    screens::fakeInput("Card number", "4000 0000 0000 0002");
    screens::fakeInput("Name on card", "TANAYA THORAT");
    screens::fakeInput("Expiry", "08/29");
    screens::fakeInput("CVV", "123", true);
    Terminal::write("\n");
    CardPayment declined(b.total, "4000 0000 0000 0002", "TANAYA THORAT", "08/29", "123");
    anim::spinner(declined.processingSteps()[0], 800);
    try {
        store_.placeOrder(*user_, cart_, declined, user_->getAddress());
    } catch (const PaymentException& e) {
        Terminal::write("\n");
        anim::failureShake(e.title(), e.what());
    }
    beat(1600);

    // Attempt 2: UPI succeeds.
    frame("Checkout › Payment", payHead);
    pay.animateTo(0);
    screens::fakeInput("UPI ID", "tanaya@okaxis");
    Terminal::write("\n");
    UpiPayment upi(b.total, "tanaya@okaxis");
    std::vector<std::string> steps = upi.processingSteps();
    anim::spinner(steps[0], 900);
    CheckoutResult result = store_.placeOrder(*user_, cart_, upi, user_->getAddress());
    for (std::size_t i = 1; i < steps.size(); ++i) anim::spinner(steps[i], 750);
    beat(300);

    const Order& o = *result.order;
    frame("Checkout › Done", cap + screens::stepper(3));
    anim::successBurst("Order #" + toStr(o.getId()) + " placed!",
                       o.total().str() + " via UPI  ·  ref " + o.getPaymentRef());
    Terminal::write("\n");
    if (!result.invoicePath.empty()) {
        screens::notify(Tone::Success, "Invoice saved to " + result.invoicePath);
    } else {
        screens::notify(Tone::Warning, "Invoice not saved: " + result.invoiceError);
    }
    screens::notify(Tone::Info, "You saved " + o.totals().totalSavings().str() + " on this order.");
    beat(2600);
}

void Demo::sceneOrders() {
    ++scene_;
    std::string cap = caption("Order history & invoice", "orders persisted to CSV, invoice written to a file");
    std::vector<OrderPtr> mine = store_.ordersOf(user_->getId());
    std::string body = cap + "  " + style::muted("Total spent ") +
                       style::paintBold(store_.totalSpent(user_->getId()).str(), theme::kSecondary) +
                       style::muted("  ·  " + toStr(mine.size()) + " orders") + "\n\n" +
                       screens::orderTable(mine, 0, false);
    frame("My orders", body);
    beat(2000);
    if (mine.empty()) return;
    frame("My orders › Detail", cap + screens::orderDetail(*mine[0]));
    beat(2600);

    std::string text = store_.invoices().render(*mine[0], user_.get());
    std::vector<std::string> lines = util::split(text, '\n');
    if (!lines.empty() && lines.back().empty()) lines.pop_back();
    std::vector<std::string> painted;
    for (const std::string& l : lines) painted.push_back(style::paint(l, theme::kText));
    frame("Invoice", cap + box(painted, 66, "INV-" + toStr(mine[0]->getId()) + ".txt", theme::kGold, 2));
    beat(2600);
}

void Demo::sceneAdmin() {
    ++scene_;
    std::string cap = caption("Admin sales report", "STL map/accumulate aggregated into ASCII charts");
    user_.reset();
    cart_.clear();
    frame("Admin login", cap);
    screens::sectionTitle("Admin console", "authorised staff only");
    screens::fakeInput("Username", "admin");
    screens::fakeInput("Password", "admin123", true);
    const Admin& a = store_.auth().adminLogin("admin", "admin123");
    anim::spinner("Verifying credentials", 600);
    screens::notify(Tone::Success, a.greeting());
    beat(700);

    SalesReport r = store_.salesReport();
    frame("Admin › Dashboard", cap, true);
    Terminal::write(screens::kpiTiles(r, store_.products().size(), store_.customers().size()) + "\n");
    screens::animatedSalesCharts(r);
    Terminal::write("\n" + screens::lowStockPanel(store_.lowStock()));
    beat(3800);
}

void Demo::outro() {
    Terminal::clear();
    Terminal::write("\n\n");
    if (Terminal::animationEnabled()) {
        for (int f = 0; f < 18; ++f) {
            if (f > 0) Terminal::cursorUp(6);
            Terminal::write(indentLines(screens::logoLines(0.5 + f * 0.05), 2 + (kPageWidth - 81) / 2));
            Terminal::flush();
            Terminal::sleepMs(40);
        }
    } else {
        Terminal::write(indentLines(screens::logoLines(0.5), 2 + (kPageWidth - 81) / 2));
    }
    Terminal::write("\n");
    std::string line1 = "That was ShopSphere - thanks for watching!";
    anim::typewriter(style::paintBold(line1, kWhite), 24, 2 + (kPageWidth - static_cast<int>(line1.size())) / 2);
    std::string line2 = "Run  bin/shopsphere  without --demo to explore every screen.";
    Terminal::write(std::string(static_cast<std::size_t>(2 + (kPageWidth - static_cast<int>(line2.size())) / 2), ' ') +
                    style::muted(line2) + "\n\n");
    Terminal::flush();
}

}  // namespace ui
}  // namespace shop
