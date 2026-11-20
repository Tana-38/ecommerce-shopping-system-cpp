#include "ui/App.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "payment/CardPayment.h"
#include "payment/CashOnDelivery.h"
#include "payment/UpiPayment.h"
#include "payment/WalletPayment.h"
#include "ui/Animation.h"
#include "ui/Chart.h"
#include "ui/Color.h"
#include "ui/Menu.h"
#include "ui/Table.h"
#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {

using util::toStr;

namespace {
const RGB kWhite{255, 255, 255};

int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// A one-line message shown under a list screen until the next key press.
struct Flash {
    std::string text;
    Tone tone = Tone::Info;
    void set(Tone t, const std::string& s) {
        tone = t;
        text = s;
    }
    std::string render() {
        if (text.empty()) return "\n";
        RGB c = tone == Tone::Success ? theme::kSuccess
                : tone == Tone::Error ? theme::kDanger
                : tone == Tone::Warning ? theme::kWarning
                                        : theme::kInfo;
        std::string icon = tone == Tone::Success ? "✔" : tone == Tone::Error ? "✘" : "!";
        std::string out = "  " + style::paintBold(" " + icon + " ", c) + style::paint(text, c) + "\n";
        text.clear();
        return out;
    }
};
}  // namespace

App::App(Store& store) : store_(store), admin_(false) {}

// ============================================================================
// Chrome
// ============================================================================

std::string App::sessionInfo() const {
    if (admin_) return style::paint("● ", theme::kGold) + "admin  ·  Administrator";
    std::string who = user_ ? user_->firstName() : "Guest";
    std::string dot = style::paint("● ", user_ && !user_->isGuest() ? theme::kSuccess : theme::kMuted);
    std::string cart = "Cart " + toStr(cart_.units());
    if (!cart_.isEmpty()) cart += " · " + cart_.total().shortStr();
    return dot + who + style::paint("  │  ", theme::kFaint) + cart;
}

void App::present(const std::string& crumb, const std::string& body, const KeyHints& keys) const {
    std::string frame = screens::header(crumb, sessionInfo()) + "\n" + body;
    if (!keys.empty()) frame += screens::footer(keys);
    if (Terminal::vtEnabled()) {
        // Repaint in place (no flicker): home, overwrite each line, clear the rest.
        frame = "\x1b[H" + util::replaceAll(frame, "\n", "\x1b[K\n") + "\x1b[J";
    } else {
        frame = "\n" + frame;
    }
    Terminal::write(frame);
    Terminal::flush();
}

bool App::askInt(const std::string& label, int lo, int hi, int& out, const std::string& hint) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        std::string s = screens::prompt(label, hint.empty() ? toStr(lo) + "-" + toStr(hi) : hint);
        if (s.empty()) return false;
        int v = 0;
        // OOP: exception handling -- throw on bad input, catch and re-prompt.
        try {
            if (!util::parseInt(s, v)) throw InvalidInputException("'" + s + "' is not a whole number.");
            if (v < lo || v > hi) {
                throw InvalidInputException("Please enter a value between " + toStr(lo) + " and " +
                                            toStr(hi) + ".");
            }
            out = v;
            return true;
        } catch (const InvalidInputException& e) {
            screens::showError(e);
        }
    }
    return false;
}

bool App::askDouble(const std::string& label, double lo, double hi, double& out,
                    const std::string& hint) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        std::string s = screens::prompt(label, hint);
        if (s.empty()) return false;
        double v = 0;
        if (util::parseDouble(s, v) && v >= lo && v <= hi) {
            out = v;
            return true;
        }
        screens::notify(Tone::Error, "Please enter a number between " + util::fixed(lo, 0) + " and " +
                                         util::fixed(hi, 0) + ".");
    }
    return false;
}

// ============================================================================
// Entry point & auth
// ============================================================================

int App::run() {
    Money::setSymbol("₹");
    screens::splash(true);
    if (store_.skippedRows() > 0) {
        screens::notify(Tone::Warning, toStr(store_.skippedRows()) +
                                           " malformed row(s) in data/*.csv were skipped.");
    }
    screens::waitKey("Press any key to start shopping");
    try {
        while (authMenu()) {
        }
    } catch (const InputClosed&) {
        // stdin closed (EOF / Ctrl+C) -- fall through to a clean exit.
    }
    Terminal::write("\n  " + style::gradient("Thanks for visiting ShopSphere. See you soon!",
                                             theme::kSecondary, theme::kAccent) +
                    "\n\n");
    return 0;
}

bool App::authMenu() {
    present("Welcome", "", KeyHints());
    std::vector<std::string> logo = screens::logoLines(0.1);
    Terminal::write(indentLines(logo, 2 + (kPageWidth - 81) / 2) + "\n");
    Menu menu("Get started", {MenuItem("Log in", "existing customer"),
                              MenuItem("Create an account", "₹500 welcome bonus"),
                              MenuItem("Continue as guest", "browse & build a cart"),
                              MenuItem("Admin console", "staff only"),
                              MenuItem("Exit", "")});
    Terminal::write(style::muted("  Demo account: tanaya@shop.com / pass123   ·   Admin: admin / admin123") +
                    "\n\n");
    int choice = menu.run();
    switch (choice) {
        case 0:
            if (loginFlow()) customerHome();
            return true;
        case 1:
            if (registerFlow()) customerHome();
            return true;
        case 2:
            user_.reset();
            admin_ = false;
            customerHome();
            return true;
        case 3:
            if (adminLoginFlow()) adminHome();
            return true;
        default:
            return false;
    }
}

bool App::loginFlow() {
    for (int attempt = 1; attempt <= 3; ++attempt) {
        present("Log in", "", KeyHints{{"Enter", "submit"}, {"blank", "cancel"}});
        screens::sectionTitle("Welcome back", "attempt " + toStr(attempt) + " of 3");
        std::string email = screens::prompt("Email");
        if (email.empty()) return false;
        std::string password = screens::promptSecret("Password");
        Terminal::write("\n");
        try {
            std::shared_ptr<Customer> c = store_.auth().login(email, password);
            anim::spinner("Signing you in securely", 700);
            user_ = c;
            admin_ = false;
            screens::notify(Tone::Success, c->greeting() + "! Wallet balance " + c->getWallet().str());
            Terminal::sleepMs(700);
            return true;
        } catch (const AuthException& e) {   // OOP: exception handling
            screens::showError(e);
            screens::waitKey();
        }
    }
    screens::notify(Tone::Error, "Too many failed attempts. Returning to the start screen.");
    screens::waitKey();
    return false;
}

bool App::registerFlow() {
    present("Create account", "", KeyHints{{"blank", "cancel"}});
    screens::sectionTitle("Join ShopSphere", "get ₹500 in your wallet on sign-up");
    std::string name = screens::prompt("Full name");
    if (name.empty()) return false;
    std::string email = screens::prompt("Email", "name@example.com");
    if (email.empty()) return false;
    std::string phone = screens::prompt("Mobile", "10 digits");
    if (phone.empty()) return false;
    std::string password = screens::promptSecret("Password (6+ chars, letters & digits)");
    std::string again = screens::promptSecret("Confirm password");
    std::string address = screens::prompt("Delivery address");
    Terminal::write("\n");
    try {
        if (password != again) throw InvalidInputException("Passwords do not match.");
        std::shared_ptr<Customer> c =
            store_.auth().registerCustomer(name, email, phone, password, address);
        store_.saveCustomers();
        anim::spinner("Creating your account", 700);
        user_ = c;
        admin_ = false;
        screens::notify(Tone::Success, "Account created! " + AuthService::welcomeBonus().str() +
                                           " has been added to your wallet.");
        screens::waitKey();
        return true;
    } catch (const ShopException& e) {
        screens::showError(e);
        screens::waitKey();
        return false;
    }
}

bool App::adminLoginFlow() {
    present("Admin login", "", KeyHints{{"blank", "cancel"}});
    screens::sectionTitle("Admin console", "authorised staff only");
    std::string user = screens::prompt("Username");
    if (user.empty()) return false;
    std::string pw = screens::promptSecret("Password");
    try {
        const Admin& a = store_.auth().adminLogin(user, pw);
        anim::spinner("Verifying credentials", 600);
        admin_ = true;
        user_.reset();
        screens::notify(Tone::Success, a.greeting());
        Terminal::sleepMs(600);
        return true;
    } catch (const AuthException& e) {
        screens::showError(e);
        screens::waitKey();
        return false;
    }
}

void App::logout() {
    user_.reset();
    admin_ = false;
    cart_.clear();
}

// ============================================================================
// Customer area
// ============================================================================

void App::customerHome() {
    int last = 0;
    for (;;) {
        bool guest = !user_;
        std::string cartHint = cart_.isEmpty() ? "empty"
                                               : toStr(cart_.units()) + " items · " + cart_.total().shortStr();
        std::size_t orderCount = guest ? 0 : store_.ordersOf(user_->getId()).size();
        std::vector<MenuItem> items = {
            MenuItem("Browse catalog", toStr(store_.products().size()) + " products"),
            MenuItem("Search products", "by name or type"),
            MenuItem("My cart", cartHint),
            MenuItem("Checkout", cart_.isEmpty() ? "" : cart_.total().shortStr(), !cart_.isEmpty()),
            MenuItem("My orders", guest ? "log in first" : toStr(orderCount) + " orders", !guest),
            MenuItem("Profile & wallet", guest ? "log in first" : user_->getWallet().shortStr(), !guest),
            MenuItem(guest ? "Back to start" : "Log out", ""),
        };
        present("Home", "", KeyHints{{"↑↓", "move"}, {"Enter", "select"}, {"1-7", "jump"}, {"Esc", "back"}});
        std::string hello = guest ? "Hello, guest shopper" : "Hello, " + user_->firstName();
        screens::sectionTitle(hello, "what would you like to do today?");
        Menu menu("Main menu", items);
        int choice = menu.run(last);
        if (choice >= 0) last = choice;
        switch (choice) {
            case 0: browse("", false); break;
            case 1: {
                std::string term = screens::prompt("Search for", "e.g. shirt, phone, rice");
                browse(term, false);
                break;
            }
            case 2: cartScreen(); break;
            case 3: checkout(); break;
            case 4: ordersScreen(); break;
            case 5: profileScreen(); break;
            default:
                if (!guest && !screens::confirm("Log out now?")) break;
                logout();
                return;
        }
    }
}

void App::browse(const std::string& initialSearch, bool adminMode) {
    std::string category = "All";
    SortKey sort = SortKey::Featured;
    std::string search = initialSearch;
    int selected = 0;
    int offset = 0;
    const int pageSize = 12;
    Flash flash;
    for (;;) {
        // OOP: STL + chained const queries over shared products.
        std::vector<ProductPtr> items =
            store_.catalog().inCategory(category).matching(search).sortedBy(sort).items();
        int n = static_cast<int>(items.size());
        selected = n == 0 ? 0 : clampInt(selected, 0, n - 1);
        if (selected < offset) offset = selected;
        if (selected >= offset + pageSize) offset = selected - pageSize + 1;
        offset = clampInt(offset, 0, std::max(0, n - pageSize));

        std::string body = screens::filterBar(category, sort, search, items.size()) + "\n" +
                           screens::productTable(items, selected, offset, pageSize) + flash.render();
        KeyHints keys = adminMode
                            ? KeyHints{{"↑↓", "move"}, {"Enter", "details"}, {"R", "restock"},
                                       {"P", "price"}, {"D", "delete"}, {"N", "new"}, {"F", "category"},
                                       {"Esc", "back"}}
                            : KeyHints{{"↑↓", "move"}, {"Enter", "details"}, {"A", "add"}, {"F", "category"},
                                       {"S", "sort"}, {"/", "search"}, {"C", "cart"}, {"Esc", "back"}};
        present(adminMode ? "Admin › Products" : "Catalog", body, keys);

        Key k = Terminal::readKey();
        ProductPtr current = n > 0 ? items[static_cast<std::size_t>(selected)] : ProductPtr();
        std::vector<std::string> cats = Catalog::categories();
        std::size_t catIndex = static_cast<std::size_t>(
            std::find(cats.begin(), cats.end(), category) - cats.begin());
        if (k.code == KeyCode::Up || k.is('k')) {
            --selected;
        } else if (k.code == KeyCode::Down || k.is('j')) {
            ++selected;
        } else if (k.code == KeyCode::PageUp) {
            selected -= pageSize;
        } else if (k.code == KeyCode::PageDown) {
            selected += pageSize;
        } else if (k.code == KeyCode::Home) {
            selected = 0;
        } else if (k.code == KeyCode::End) {
            selected = n - 1;
        } else if (k.code == KeyCode::Right || k.code == KeyCode::Tab || k.is('f')) {
            category = cats[(catIndex + 1) % cats.size()];
            selected = 0;
        } else if (k.code == KeyCode::Left) {
            category = cats[(catIndex + cats.size() - 1) % cats.size()];
            selected = 0;
        } else if (k.is('s')) {
            sort = Catalog::nextSort(sort);
        } else if (k.is('/')) {
            search = screens::prompt("Search for", "blank clears");
            selected = 0;
        } else if (k.is('x')) {
            category = "All";
            search.clear();
            sort = SortKey::Featured;
        } else if (k.code == KeyCode::Escape || k.is('q') || k.is('b')) {
            return;
        } else if (!current) {
            continue;
        } else if (k.code == KeyCode::Enter) {
            productDetail(current, adminMode);
        } else if (!adminMode && k.is('a')) {
            try {
                store_.addToCart(cart_, current->getId(), 1);
                flash.set(Tone::Success, "Added 1 x " + current->getName() + " to your cart.");
            } catch (const ShopException& e) {
                flash.set(Tone::Error, e.what());
            }
        } else if (!adminMode && k.is('c')) {
            cartScreen();
        } else if (adminMode && k.is('r')) {
            adminRestock(current);
        } else if (adminMode && k.is('p')) {
            adminPrice(current);
        } else if (adminMode && k.is('d')) {
            adminRemove(current);
        } else if (adminMode && k.is('n')) {
            adminAddProduct();
        }
    }
}

void App::productDetail(const ProductPtr& p, bool adminMode) {
    for (;;) {
        present(adminMode ? "Admin › Product" : "Catalog › Product",
                screens::productCard(*p, cart_.quantityOf(p->getId())) + "\n", KeyHints());
        std::vector<MenuItem> items;
        if (adminMode) {
            items = {MenuItem("Restock", "add units"), MenuItem("Change price", "MRP"),
                     MenuItem("Remove from catalog", ""), MenuItem("Back", "")};
        } else {
            items = {MenuItem("Add to cart", p->getStock() > 0 ? "choose quantity" : "out of stock",
                              p->getStock() > 0),
                     MenuItem("Buy now", "add 1 & go to cart", p->getStock() > 0), MenuItem("Back", "")};
        }
        Menu menu("Actions", items, 40);
        int choice = menu.run();
        if (adminMode) {
            if (choice == 0) adminRestock(p);
            else if (choice == 1) adminPrice(p);
            else if (choice == 2) {
                adminRemove(p);
                return;
            } else return;
        } else {
            if (choice == 0) {
                addToCartFlow(p);
            } else if (choice == 1) {
                try {
                    store_.addToCart(cart_, p->getId(), 1);
                    cartScreen();
                    return;
                } catch (const ShopException& e) {
                    screens::showError(e);
                    screens::waitKey();
                }
            } else {
                return;
            }
        }
    }
}

void App::addToCartFlow(const ProductPtr& p) {
    int already = cart_.quantityOf(p->getId());
    int maxQty = p->getStock() - already;
    if (maxQty <= 0) {
        screens::notify(Tone::Warning, "You already have all available units in your cart.");
        screens::waitKey();
        return;
    }
    int qty = 0;
    if (!askInt("Quantity", 1, std::min(maxQty, 10), qty, "1-" + toStr(std::min(maxQty, 10)))) return;
    try {
        store_.addToCart(cart_, p->getId(), qty);   // may throw OutOfStockException
        anim::spinner("Adding to your cart", 400);
        screens::notify(Tone::Success, toStr(qty) + " x " + p->getName() + " added. Cart total " +
                                           cart_.total().str());
    } catch (const ShopException& e) {
        screens::showError(e);
    }
    screens::waitKey();
}

void App::cartScreen() {
    int selected = 0;
    Flash flash;
    for (;;) {
        int n = static_cast<int>(cart_.lineCount());
        selected = n == 0 ? 0 : clampInt(selected, 0, n - 1);
        std::string body = "";
        body += screens::cartTable(cart_, n == 0 ? -1 : selected) + "\n" + screens::cartSummary(cart_) +
                flash.render();
        present("Cart", body,
                KeyHints{{"↑↓", "move"}, {"+/-", "qty"}, {"R", "remove"}, {"P", "coupon"},
                         {"X", "drop coupon"}, {"E", "empty"}, {"Enter", "checkout"}, {"Esc", "back"}});
        Key k = Terminal::readKey();
        const CartItem* item = n > 0 ? &cart_.items()[static_cast<std::size_t>(selected)] : nullptr;
        try {
            if (k.code == KeyCode::Up || k.is('k')) {
                --selected;
            } else if (k.code == KeyCode::Down || k.is('j')) {
                ++selected;
            } else if (k.code == KeyCode::Escape || k.is('q') || k.is('b')) {
                return;
            } else if (k.is('p')) {
                couponFlow();
            } else if (k.is('x')) {
                if (cart_.hasCoupon()) {
                    cart_.removeCoupon();
                    flash.set(Tone::Info, "Coupon removed.");
                }
            } else if (k.is('e')) {
                if (n > 0 && screens::confirm("Remove everything from your cart?")) {
                    cart_.clear();
                    flash.set(Tone::Info, "Cart emptied.");
                }
            } else if (k.code == KeyCode::Enter || k.is('o')) {
                if (n == 0) {
                    flash.set(Tone::Warning, "Your cart is empty.");
                } else {
                    checkout();
                    if (cart_.isEmpty()) return;   // order placed
                }
            } else if (item && (k.is('+') || k.is('='))) {
                cart_.updateQuantity(item->productId(), item->quantity() + 1);
            } else if (item && k.is('-')) {
                int id = item->productId();
                std::string name = item->product()->getName();
                cart_.updateQuantity(id, item->quantity() - 1);   // 0 removes the line
                if (!cart_.contains(id)) flash.set(Tone::Info, name + " removed.");
            } else if (item && (k.is('r') || k.is('d'))) {
                std::string name = item->product()->getName();
                cart_ -= item->productId();   // OOP: operator overloading (Cart -=)
                flash.set(Tone::Info, name + " removed from cart.");
            }
        } catch (const ShopException& e) {   // e.g. OutOfStockException on '+'
            flash.set(Tone::Error, e.what());
        }
    }
}

void App::couponFlow() {
    std::string code = screens::prompt("Coupon code", "SAVE10 / FLAT200 / FESTIVE25");
    if (code.empty()) return;
    try {
        cart_.applyCoupon(code);
        PriceBreakdown b = cart_.breakdown();
        anim::spinner("Applying " + util::toUpper(code), 450);
        screens::notify(Tone::Success, "Coupon applied - you save " + b.couponDiscount.str() + " more!");
    } catch (const CouponException& e) {
        screens::showError(e);
    }
    screens::waitKey();
}

std::unique_ptr<Payment> App::buildPayment(int method, const Money& total) {
    // OOP: smart pointers + runtime polymorphism -- the caller only sees Payment.
    switch (method) {
        case 0: {
            std::string id = screens::prompt("UPI ID", "e.g. name@okaxis");
            if (id.empty()) return nullptr;
            return std::unique_ptr<Payment>(new UpiPayment(total, id));
        }
        case 1: {
            screens::notify(Tone::Info, "Test cards: 4111 1111 1111 1111 (approved), 4000 0000 0000 0002 (declined)");
            std::string number = screens::prompt("Card number", "16 digits");
            if (number.empty()) return nullptr;
            std::string holder = screens::prompt("Name on card");
            std::string expiry = screens::prompt("Expiry", "MM/YY");
            std::string cvv = screens::promptSecret("CVV");
            return std::unique_ptr<Payment>(new CardPayment(total, number, holder, expiry, cvv));
        }
        case 2:
            return std::unique_ptr<Payment>(new CashOnDelivery(total));
        case 3:
            return std::unique_ptr<Payment>(new WalletPayment(total, *user_));
        default:
            return nullptr;
    }
}

void App::checkout() {
    if (cart_.isEmpty()) {
        screens::notify(Tone::Warning, "Your cart is empty - add something first.");
        screens::waitKey();
        return;
    }
    if (!user_) {
        present("Checkout", "", KeyHints());
        screens::notify(Tone::Info, "Please log in or create an account to place your order.");
        Terminal::write("\n");
        Menu m("Continue as", {MenuItem("Log in", ""), MenuItem("Create an account", ""), MenuItem("Back", "")});
        int c = m.run();
        bool ok = (c == 0 && loginFlow()) || (c == 1 && registerFlow());
        if (!ok) return;
    }

    // ---- Step 1: address ------------------------------------------------------
    PriceBreakdown b = cart_.breakdown();
    std::string summary = indentLines(boxLines(screens::breakdownLines(b, 40), 40, "Order summary",
                                               theme::kSecondary), 2);
    present("Checkout", screens::stepper(0) + summary + "\n", KeyHints());
    Menu addrMenu("Deliver to", {MenuItem(util::truncate(user_->getAddress(), 38), "saved"),
                                 MenuItem("A different address", ""), MenuItem("Back to cart", "")}, 56);
    int a = addrMenu.run();
    std::string address = user_->getAddress();
    if (a == 1) {
        address = screens::prompt("Delivery address");
        if (util::trim(address).size() < 5) {
            screens::notify(Tone::Error, "That address looks too short.");
            screens::waitKey();
            return;
        }
    } else if (a != 0) {
        return;
    }

    // ---- Step 2: payment -------------------------------------------------------
    for (;;) {
        b = cart_.breakdown();
        std::string payBody = screens::stepper(1) + "  " + style::muted("Amount payable  ") +
                              style::paintBold(b.total.str(), theme::kSecondary) +
                              style::muted("   ·   delivering to " + util::truncate(address, 40)) +
                              "\n\n";
        present("Checkout › Payment", payBody, KeyHints());
        Menu payMenu("Pay with", {MenuItem("UPI", "GPay / PhonePe / Paytm"),
                                  MenuItem("Credit / Debit card", "Visa, Mastercard, RuPay"),
                                  MenuItem("Cash on Delivery", "up to " + CashOnDelivery::limit().shortStr()),
                                  MenuItem("ShopSphere Wallet", "balance " + user_->getWallet().shortStr()),
                                  MenuItem("Back to cart", "")}, 56);
        int method = payMenu.run();
        if (method < 0 || method > 3) return;
        std::unique_ptr<Payment> payment = buildPayment(method, b.total);
        if (!payment) continue;

        Terminal::write("\n");
        std::vector<std::string> steps = payment->processingSteps();   // OOP: runtime polymorphism
        try {
            anim::spinner(steps[0], 800);
            CheckoutResult result = store_.placeOrder(*user_, cart_, *payment, address);
            for (std::size_t i = 1; i < steps.size(); ++i) anim::spinner(steps[i], 700);
            Terminal::write("\n");
            Terminal::sleepMs(200);

            // ---- Step 3: confirmation ---------------------------------------------
            present("Checkout › Done", screens::stepper(3), KeyHints());
            const Order& o = *result.order;
            anim::successBurst("Order #" + toStr(o.getId()) + " placed!",
                               o.total().str() + " via " + payment->methodName() + "  ·  ref " +
                                   o.getPaymentRef());
            Terminal::write("\n");
            if (!result.invoicePath.empty()) {
                screens::notify(Tone::Success, "Invoice saved to " + result.invoicePath);
            } else {
                screens::notify(Tone::Warning, "Order placed, but the invoice could not be saved: " +
                                                   result.invoiceError);
            }
            PriceBreakdown t = o.totals();
            if (!t.totalSavings().isZero()) {
                screens::notify(Tone::Info, "You saved " + t.totalSavings().str() + " on this order.");
            }
            if (method == 3) {
                screens::notify(Tone::Info, "Wallet balance now " + user_->getWallet().str());
            }
            screens::waitKey("Press any key to view your orders");
            ordersScreen();
            return;
        } catch (const PaymentException& e) {
            Terminal::write("\n");
            anim::failureShake(e.title(), e.what());
            Terminal::write("\n");
            if (!screens::confirm("Try a different payment method?")) return;
        } catch (const ShopException& e) {
            screens::showError(e);
            screens::waitKey();
            return;
        }
    }
}

void App::ordersScreen() {
    if (!user_) return;
    int selected = 0;
    Flash flash;
    for (;;) {
        std::vector<OrderPtr> orders = store_.ordersOf(user_->getId());
        int n = static_cast<int>(orders.size());
        selected = n == 0 ? 0 : clampInt(selected, 0, n - 1);
        std::string body = "  " + style::muted("Total spent ") +
                           style::paintBold(store_.totalSpent(user_->getId()).str(), theme::kSecondary) +
                           style::muted("  ·  " + toStr(n) + " orders") + "\n\n" +
                           screens::orderTable(orders, n == 0 ? -1 : selected, false) + flash.render();
        present("My orders", body,
                KeyHints{{"↑↓", "move"}, {"Enter", "details"}, {"C", "cancel"}, {"I", "invoice"}, {"Esc", "back"}});
        Key k = Terminal::readKey();
        OrderPtr cur = n > 0 ? orders[static_cast<std::size_t>(selected)] : OrderPtr();
        if (k.code == KeyCode::Up || k.is('k')) {
            --selected;
        } else if (k.code == KeyCode::Down || k.is('j')) {
            ++selected;
        } else if (k.code == KeyCode::Escape || k.is('q') || k.is('b')) {
            return;
        } else if (cur && k.code == KeyCode::Enter) {
            orderDetailScreen(cur, false);
        } else if (cur && k.is('i')) {
            showInvoice(*cur);
        } else if (cur && k.is('c')) {
            if (!cur->isCancellable()) {
                flash.set(Tone::Warning, "Order #" + toStr(cur->getId()) + " is " +
                                             util::toLower(statusName(cur->getStatus())) +
                                             " and can no longer be cancelled.");
            } else if (screens::confirm("Cancel order #" + toStr(cur->getId()) + "?")) {
                try {
                    Money refund = store_.cancelOrder(cur->getId(), user_->getId(), false);
                    flash.set(Tone::Success, "Order cancelled. Items restocked." +
                                                 (refund.isZero() ? std::string()
                                                                  : " " + refund.str() + " refunded to wallet."));
                } catch (const ShopException& e) {
                    flash.set(Tone::Error, e.what());
                }
            }
        }
    }
}

void App::orderDetailScreen(const OrderPtr& order, bool adminMode) {
    for (;;) {
        present(adminMode ? "Admin › Order" : "My orders › Detail", screens::orderDetail(*order) + "\n",
                KeyHints());
        std::vector<MenuItem> items = {MenuItem("View invoice", "INV-" + toStr(order->getId())),
                                       MenuItem("Cancel order", order->isCancellable() ? "restocks items" : "",
                                                order->isCancellable())};
        if (adminMode) {
            bool canAdvance = order->getStatus() == OrderStatus::Placed ||
                              order->getStatus() == OrderStatus::Shipped;
            items.push_back(MenuItem("Advance status", canAdvance ? "Placed → Shipped → Delivered" : "",
                                     canAdvance));
        }
        items.push_back(MenuItem("Back", ""));
        Menu menu("Actions", items, 46);
        int c = menu.run();
        try {
            if (c == 0) {
                showInvoice(*order);
            } else if (c == 1) {
                if (screens::confirm("Cancel order #" + toStr(order->getId()) + "?")) {
                    Money refund = store_.cancelOrder(order->getId(), user_ ? user_->getId() : 0, adminMode);
                    screens::notify(Tone::Success, "Order cancelled." + (refund.isZero()
                                                                              ? std::string()
                                                                              : " Refunded " + refund.str() +
                                                                                    " to the customer's wallet."));
                    screens::waitKey();
                }
            } else if (adminMode && c == 2) {
                store_.advanceOrder(order->getId());
                screens::notify(Tone::Success, "Order is now " + statusName(order->getStatus()) + ".");
                screens::waitKey();
            } else {
                return;
            }
        } catch (const ShopException& e) {
            screens::showError(e);
            screens::waitKey();
        }
    }
}

void App::showInvoice(const Order& order) {
    std::string path = store_.invoices().pathFor(order.getId());
    std::shared_ptr<Customer> owner = store_.customers().findById(order.getCustomerId());
    std::string text = store_.invoices().render(order, owner.get());
    std::vector<std::string> lines = util::split(text, '\n');
    if (!lines.empty() && lines.back().empty()) lines.pop_back();
    std::vector<std::string> painted;
    for (const std::string& l : lines) painted.push_back(style::paint(l, theme::kText));
    std::string note = std::ifstream(path.c_str()).good() ? "saved at " + path : "preview (no file yet)";
    present("Invoice", box(painted, 66, "INV-" + toStr(order.getId()) + "  ·  " + note, theme::kGold, 2),
            KeyHints());
    screens::waitKey();
}

void App::profileScreen() {
    if (!user_) return;
    for (;;) {
        present("Profile",
                screens::profileCard(*user_, static_cast<int>(store_.ordersOf(user_->getId()).size()),
                                     store_.totalSpent(user_->getId())) + "\n",
                KeyHints());
        Menu menu("Edit", {MenuItem("Change name", ""), MenuItem("Change mobile number", "10 digits"),
                           MenuItem("Change address", ""), MenuItem("Change password", ""),
                           MenuItem("Add money to wallet", "up to ₹10,000"), MenuItem("Back", "")}, 46);
        int c = menu.run();
        try {
            if (c == 0) {
                std::string v = screens::prompt("New name");
                if (v.empty()) continue;
                AuthService::validateName(v);
                user_->setName(v);
            } else if (c == 1) {
                std::string v = screens::prompt("New mobile number");
                if (v.empty()) continue;
                // OOP: encapsulation -- the setter refuses invalid data.
                if (!user_->setPhone(v)) throw InvalidInputException("Mobile number must be exactly 10 digits.");
            } else if (c == 2) {
                std::string v = screens::prompt("New address");
                if (util::trim(v).size() < 5) throw InvalidInputException("Address is too short.");
                user_->setAddress(v);
            } else if (c == 3) {
                std::string current = screens::promptSecret("Current password");
                if (!user_->checkPassword(current)) throw AuthException("Current password is incorrect.");
                std::string next = screens::promptSecret("New password");
                AuthService::validatePassword(next);
                user_->setPassword(next);
            } else if (c == 4) {
                double amount = 0;
                if (!askDouble("Amount to add (₹)", 1, 10000, amount, "1-10000")) continue;
                user_->creditWallet(Money(amount));
                anim::spinner("Adding money via UPI autopay", 600);
            } else {
                return;
            }
            store_.saveCustomers();
            screens::notify(Tone::Success, "Profile updated.");
            Terminal::sleepMs(500);
        } catch (const ShopException& e) {
            screens::showError(e);
            screens::waitKey();
        }
    }
}

// ============================================================================
// Admin area
// ============================================================================

void App::adminHome() {
    int last = 0;
    for (;;) {
        std::size_t low = store_.lowStock().size();
        present("Admin", "", KeyHints{{"↑↓", "move"}, {"Enter", "select"}, {"Esc", "log out"}});
        screens::sectionTitle("Admin console", util::nowStamp());
        Menu menu("Manage store",
                  {MenuItem("Dashboard & sales report", "charts"),
                   MenuItem("Products & stock", toStr(store_.products().size()) + " items"),
                   MenuItem("Add a product", "any category"),
                   MenuItem("All orders", toStr(store_.orders().size()) + " orders"),
                   MenuItem("Low-stock alerts", low ? toStr(low) + " need attention" : "all good"),
                   MenuItem("Customers", toStr(store_.customers().size()) + " accounts"),
                   MenuItem("Log out", "")});
        int c = menu.run(last);
        if (c >= 0) last = c;
        switch (c) {
            case 0: adminDashboard(); break;
            case 1: browse("", true); break;
            case 2: adminAddProduct(); break;
            case 3: adminOrders(); break;
            case 4: adminLowStock(); break;
            case 5: adminCustomers(); break;
            default:
                logout();
                return;
        }
    }
}

void App::adminDashboard() {
    SalesReport r = store_.salesReport();
    present("Admin › Dashboard", "", KeyHints());
    Terminal::write(screens::kpiTiles(r, store_.products().size(), store_.customers().size()) + "\n");
    screens::animatedSalesCharts(r);
    Terminal::write("\n" + screens::lowStockPanel(store_.lowStock()));
    Terminal::write("  " + style::muted("Live product objects in memory: " + toStr(Product::liveCount()) +
                                        "   ·   orders placed this session: " +
                                        toStr(Order::placedThisSession())) +
                    "\n");
    screens::waitKey();
}

void App::adminAddProduct() {
    present("Admin › Add product", "", KeyHints{{"blank", "cancel"}});
    Menu typeMenu("Category", {MenuItem("Electronics", "10% off, warranty"),
                               MenuItem("Smartphone", "15% off, storage + warranty"),
                               MenuItem("Clothing", "5% / 20% off, size"),
                               MenuItem("Grocery", "flat ₹20 off, expiry"), MenuItem("Cancel", "")}, 46);
    int t = typeMenu.run();
    if (t < 0 || t > 3) return;
    const char* types[4] = {"Electronics", "Smartphone", "Clothing", "Grocery"};
    std::string name = screens::prompt("Product name");
    if (name.empty()) return;
    double price = 0;
    if (!askDouble("MRP (₹)", 1, 10000000, price, "e.g. 1499")) return;
    int stock = 0;
    if (!askInt("Opening stock", 0, 100000, stock)) return;
    double rating = 4.0;
    askDouble("Rating", 0, 5, rating, "0-5, blank = 4.0");
    std::vector<std::string> attrs;
    if (t == 0 || t == 1) attrs.push_back(screens::prompt("Warranty (months)"));
    if (t == 1) attrs.push_back(screens::prompt("Storage (GB)"));
    if (t == 2) attrs.push_back(screens::prompt("Size", "S/M/L/XL/32/Free"));
    if (t == 3) attrs.push_back(screens::prompt("Best before", "YYYY-MM-DD"));
    try {
        ProductPtr p = store_.addProduct(types[t], name, price, stock, attrs, rating);
        anim::spinner("Publishing to catalog", 500);
        Terminal::write("\n" + screens::productCard(*p, 0));
        screens::notify(Tone::Success, "Product #" + toStr(p->getId()) + " is now live.");
    } catch (const ShopException& e) {
        screens::showError(e);
    }
    screens::waitKey();
}

void App::adminRestock(const ProductPtr& p) {
    Terminal::write("\n");
    int qty = 0;
    if (!askInt("Units to add to '" + p->getName() + "'", 1, 10000, qty)) return;
    try {
        store_.restock(p->getId(), qty);
        screens::notify(Tone::Success, p->getName() + " now has " + toStr(p->getStock()) + " in stock.");
    } catch (const ShopException& e) {
        screens::showError(e);
    }
    screens::waitKey();
}

void App::adminPrice(const ProductPtr& p) {
    Terminal::write("\n");
    double price = 0;
    if (!askDouble("New MRP for '" + p->getName() + "' (₹)", 1, 10000000, price,
                   "now " + Money(p->getPrice()).shortStr())) {
        return;
    }
    try {
        store_.updatePrice(p->getId(), price);
        screens::notify(Tone::Success, "Price updated. Customers now pay " +
                                           Money(p->getFinalPrice()).str() + " after the " +
                                           p->getCategory() + " offer.");
    } catch (const ShopException& e) {
        screens::showError(e);
    }
    screens::waitKey();
}

void App::adminRemove(const ProductPtr& p) {
    Terminal::write("\n");
    if (!screens::confirm("Remove '" + p->getName() + "' from the catalog?")) return;
    try {
        store_.removeProduct(p->getId());
        screens::notify(Tone::Success, "Removed. Past orders keep their own copy of the item.");
    } catch (const ShopException& e) {
        screens::showError(e);
    }
    screens::waitKey();
}

void App::adminOrders() {
    int selected = 0;
    for (;;) {
        std::vector<OrderPtr> orders = store_.allOrdersNewestFirst();
        int n = static_cast<int>(orders.size());
        selected = n == 0 ? 0 : clampInt(selected, 0, n - 1);
        // Show a window of 14 orders around the selection.
        int offset = clampInt(selected - 6, 0, std::max(0, n - 14));
        std::vector<OrderPtr> window(orders.begin() + offset,
                                     orders.begin() + std::min(n, offset + 14));
        std::string body = "  " + style::muted(toStr(n) + " orders  ·  newest first") + "\n\n" +
                           screens::orderTable(window, n == 0 ? -1 : selected - offset, true);
        present("Admin › Orders", body,
                KeyHints{{"↑↓", "move"}, {"Enter", "details / actions"}, {"Esc", "back"}});
        Key k = Terminal::readKey();
        if (k.code == KeyCode::Up || k.is('k')) --selected;
        else if (k.code == KeyCode::Down || k.is('j')) ++selected;
        else if (k.code == KeyCode::Escape || k.is('q') || k.is('b')) return;
        else if (k.code == KeyCode::Enter && n > 0) {
            orderDetailScreen(orders[static_cast<std::size_t>(selected)], true);
        }
    }
}

void App::adminLowStock() {
    present("Admin › Low stock", screens::lowStockPanel(store_.lowStock()) + "\n", KeyHints());
    std::vector<ProductPtr> low = store_.lowStock();
    if (low.empty()) {
        screens::waitKey();
        return;
    }
    if (screens::confirm("Restock one of these now?")) {
        int id = 0;
        if (askInt("Product ID", 1, 1000000, id)) {
            ProductPtr p = store_.products().findById(id);
            if (p) {
                adminRestock(p);
            } else {
                screens::notify(Tone::Error, "No product with ID " + toStr(id) + ".");
                screens::waitKey();
            }
        }
    }
}

void App::adminCustomers() {
    Table t({{"ID", 3, Align::Right},
             {"Name", 18, Align::Left},
             {"Email", 22, Align::Left},
             {"Mobile", 10, Align::Left},
             {"Wallet", 9, Align::Right},
             {"Orders", 6, Align::Center},
             {"Spent", 10, Align::Right}});
    for (const auto& c : store_.customers()) {
        t.addRow({style::muted(toStr(c->getId())), c->getName(), c->getEmail(), c->getPhone(),
                  style::paint(c->getWallet().shortStr(), theme::kSuccess),
                  toStr(store_.ordersOf(c->getId()).size()),
                  style::paintBold(store_.totalSpent(c->getId()).shortStr(), kWhite)});
    }
    present("Admin › Customers", t.render(2), KeyHints());
    screens::waitKey();
}

}  // namespace ui
}  // namespace shop
