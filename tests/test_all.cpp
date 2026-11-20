// ShopSphere unit tests -- a tiny self-contained assert framework.
// Build:  make test     (or build.bat test / ./build.sh test)
// Prints PASS/FAIL per test case and exits non-zero if anything fails.

#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "cart/Cart.h"
#include "cart/Order.h"
#include "cart/Repository.h"
#include "core/Catalog.h"
#include "core/Clothing.h"
#include "core/Customer.h"
#include "core/Electronics.h"
#include "core/Grocery.h"
#include "core/Smartphone.h"
#include "payment/CardPayment.h"
#include "payment/CashOnDelivery.h"
#include "payment/Coupon.h"
#include "payment/UpiPayment.h"
#include "payment/WalletPayment.h"
#include "services/FileStore.h"
#include "services/InvoiceWriter.h"
#include "services/SeedData.h"
#include "services/Store.h"
#include "util/Exceptions.h"
#include "util/Money.h"
#include "util/StringUtil.h"

#ifdef _WIN32
#include <direct.h>
#define SHOP_RMDIR _rmdir
#else
#include <unistd.h>
#define SHOP_RMDIR rmdir
#endif

using namespace shop;

// ---------------------------------------------------------------------------
// Mini test framework
// ---------------------------------------------------------------------------
namespace {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

int g_checks = 0;
int g_failedChecks = 0;
bool g_currentFailed = false;

struct Registrar {
    Registrar(const std::string& name, std::function<void()> body) {
        registry().push_back({name, body});
    }
};

void check(bool ok, const std::string& what, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failedChecks;
        g_currentFailed = true;
        std::cout << "      check failed: " << what << "  (" << file << ":" << line << ")\n";
    }
}

template <typename A, typename B>
void checkEqual(const A& a, const B& b, const std::string& what, const char* file, int line) {
    bool ok = a == b;
    if (!ok) {
        std::ostringstream os;
        os << what << "  -> got [" << a << "] expected [" << b << "]";
        check(false, os.str(), file, line);
    } else {
        check(true, what, file, line);
    }
}

}  // namespace

#define TEST(name)                                         \
    static void name();                                    \
    static Registrar registrar_##name(#name, name);        \
    static void name()
#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b) checkEqual((a), (b), #a " == " #b, __FILE__, __LINE__)
#define CHECK_THROWS(expr, Type)                                              \
    do {                                                                      \
        bool thrown_ = false;                                                 \
        try {                                                                 \
            expr;                                                             \
        } catch (const Type&) {                                               \
            thrown_ = true;                                                   \
        } catch (...) {                                                       \
        }                                                                     \
        check(thrown_, "throws " #Type ": " #expr, __FILE__, __LINE__);       \
    } while (0)

static Money rs(const char* text) { return Money::parse(text); }

// ---------------------------------------------------------------------------
// Product hierarchy & discounts
// ---------------------------------------------------------------------------

TEST(discount_rules_per_category) {
    Electronics e(1, "Speaker", 1000.0, 5, 12);
    Smartphone s(2, "Phone", 10000.0, 5, 12, 128);
    Clothing expensive(3, "Jacket", 1200.0, 5, "L");
    Clothing cheap(4, "Tee", 1000.0, 5, "M");   // exactly 1000 -> 5% (rule is "> 1000")
    Grocery g(5, "Rice", 100.0, 5, "2027-01-01");
    Grocery tiny(6, "Candy", 15.0, 5, "2027-01-01");

    CHECK_EQ(Money(e.calculateDiscount()), rs("100"));
    CHECK_EQ(Money(s.calculateDiscount()), rs("1500"));   // 10% + 5% launch offer
    CHECK_EQ(Money(expensive.calculateDiscount()), rs("240"));
    CHECK_EQ(Money(cheap.calculateDiscount()), rs("50"));
    CHECK_EQ(Money(g.calculateDiscount()), rs("20"));
    CHECK_EQ(Money(tiny.calculateDiscount()), rs("15"));   // capped at the price
    CHECK_EQ(Money(tiny.getFinalPrice()), Money());
    CHECK_EQ(Money(s.getFinalPrice()), rs("8500"));
}

TEST(runtime_polymorphism_through_base_pointers) {
    std::vector<std::unique_ptr<Product>> items;
    items.push_back(std::unique_ptr<Product>(new Electronics(1, "A", 1000.0, 1, 6)));
    items.push_back(std::unique_ptr<Product>(new Smartphone(2, "B", 1000.0, 1, 6, 64)));
    items.push_back(std::unique_ptr<Product>(new Clothing(3, "C", 2000.0, 1, "S")));
    items.push_back(std::unique_ptr<Product>(new Grocery(4, "D", 50.0, 1, "2027-01-01")));
    std::vector<std::string> cats;
    double totalDiscount = 0;
    for (const auto& p : items) {
        cats.push_back(p->getCategory());
        totalDiscount += p->calculateDiscount();
    }
    CHECK_EQ(cats[0], std::string("Electronics"));
    CHECK_EQ(cats[1], std::string("Electronics"));   // a Smartphone IS-A Electronics
    CHECK_EQ(items[1]->getType(), std::string("Smartphone"));
    CHECK_EQ(cats[2], std::string("Clothing"));
    CHECK_EQ(cats[3], std::string("Grocery"));
    CHECK_EQ(Money(totalDiscount), rs("670"));      // 100 + 150 + 400 + 20

    std::unique_ptr<Product> copy = items[1]->clone();
    CHECK_EQ(copy->getType(), std::string("Smartphone"));
    CHECK_EQ(copy->getName(), std::string("B"));
}

TEST(product_validation_and_stock) {
    Electronics e(1, "Laptop", 50000.0, 5, 24);
    CHECK(e.reduceStock(3));
    CHECK_EQ(e.getStock(), 2);
    CHECK(!e.reduceStock(10));
    CHECK(!e.reduceStock(-1));
    CHECK_EQ(e.getStock(), 2);
    e.restock(8);
    CHECK_EQ(e.getStock(), 10);
    CHECK_THROWS(e.restock(0), InvalidInputException);
    CHECK_THROWS(e.setPrice(-5), InvalidInputException);
    CHECK_THROWS(e.setRating(7.5), InvalidInputException);
    e.setPrice(45000.0);
    CHECK_EQ(Money(e.getFinalPrice()), rs("40500"));
    CHECK(e.isLowStock(10));
}

TEST(static_live_object_counter) {
    int before = Product::liveCount();
    {
        Grocery a(901, "A", 10.0, 1, "2027-01-01");
        Grocery b(a);   // copy constructor also counts
        CHECK_EQ(Product::liveCount(), before + 2);
    }
    CHECK_EQ(Product::liveCount(), before);   // destructors ran
    int next = Product::generateId();
    CHECK(next > 901);
}

TEST(product_factory) {
    std::unique_ptr<Product> p = createProduct("Smartphone", 50, "Pixel", 30000.0, 4, {"12", "256"});
    CHECK_EQ(p->getType(), std::string("Smartphone"));
    CHECK_EQ(p->attributeFields().size(), static_cast<std::size_t>(2));
    CHECK_THROWS(createProduct("Furniture", 51, "Chair", 100.0, 1, {"x"}), InvalidInputException);
    CHECK_THROWS(createProduct("Electronics", 52, "TV", 100.0, 1, {"abc"}), InvalidInputException);
    CHECK_THROWS(createProduct("Clothing", 53, "Cap", -1.0, 1, {"M"}), InvalidInputException);
}

// ---------------------------------------------------------------------------
// Money & operator overloading
// ---------------------------------------------------------------------------

TEST(money_arithmetic_and_formatting) {
    Money::setSymbol("Rs ");
    CHECK_EQ(rs("1234.5").paise(), 123450LL);
    CHECK_EQ(rs("1234.5").str(), std::string("Rs 1,234.50"));
    CHECK_EQ(rs("12345678").str(), std::string("Rs 1,23,45,678.00"));
    CHECK_EQ(rs("999").shortStr(), std::string("Rs 999"));
    CHECK_EQ(rs("1619.1").shortStr(), std::string("Rs 1,619.10"));
    CHECK_EQ(rs("-20").plain(), std::string("-Rs 20.00"));
    CHECK_EQ(rs("100") + rs("0.5"), rs("100.50"));
    CHECK_EQ(rs("100") - rs("0.5"), rs("99.50"));
    CHECK_EQ(rs("19.99") * 3, rs("59.97"));
    CHECK_EQ(rs("100") * 0.18, rs("18"));
    CHECK_EQ(rs("20766.35").percent(25), rs("5191.59"));   // rounded to the paisa
    CHECK(rs("10") < rs("10.01"));
    CHECK(rs("10") >= rs("10"));
    CHECK(rs("5") != rs("5.01"));
    Money m = rs("10");
    m += rs("2.5");
    m -= rs("0.5");
    CHECK_EQ(m, rs("12"));
    CHECK_EQ(Money(0.1 + 0.2), rs("0.30"));   // no floating point drift
    std::ostringstream os;
    os << rs("1500");
    CHECK_EQ(os.str(), std::string("Rs 1,500.00"));
    CHECK_THROWS(Money::parse("12a"), InvalidInputException);
    CHECK_EQ(rs("1234.56").number(), std::string("1234.56"));
}

TEST(stream_operators_for_product_and_order) {
    Electronics e(7, "Speaker", 1000.0, 3, 12);
    std::ostringstream os;
    os << e;   // friend operator<< through a Product&
    CHECK(os.str().find("Speaker") != std::string::npos);
    CHECK(os.str().find("900.00") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Cart
// ---------------------------------------------------------------------------

TEST(cart_add_merge_remove) {
    auto rice = std::make_shared<Grocery>(1, "Rice", 100.0, 10, "2027-01-01");
    auto tee = std::make_shared<Clothing>(2, "Tee", 500.0, 3, "M");
    Cart cart;
    CHECK(cart.isEmpty());
    cart.addItem(rice, 2);
    cart += CartItem(rice, 3);   // operator+= merges with the existing line
    CHECK_EQ(cart.lineCount(), static_cast<std::size_t>(1));
    CHECK_EQ(cart.quantityOf(1), 5);
    cart.addItem(CartItem(tee, 1));
    CHECK_EQ(cart.units(), 6);
    CHECK(cart.contains(2));
    cart -= 2;                   // operator-= removes by product id
    CHECK(!cart.contains(2));
    cart.updateQuantity(1, 4);
    CHECK_EQ(cart.quantityOf(1), 4);
    cart.updateQuantity(1, 0);   // zero removes the line
    CHECK(cart.isEmpty());
    CHECK_THROWS(cart -= 99, NotFoundException);
}

TEST(cart_stock_and_quantity_validation) {
    auto tee = std::make_shared<Clothing>(2, "Tee", 500.0, 3, "M");
    Cart cart;
    CHECK_THROWS(cart.addItem(tee, 4), OutOfStockException);
    CHECK_THROWS(cart.addItem(tee, 0), InvalidInputException);
    cart.addItem(tee, 2);
    CHECK_THROWS(cart.addItem(tee, 2), OutOfStockException);   // 2 + 2 > 3
    CHECK_EQ(cart.quantityOf(2), 2);                           // unchanged after failure
    try {
        cart.updateQuantity(2, 9);
        CHECK(false);
    } catch (const OutOfStockException& e) {
        CHECK_EQ(e.available(), 3);
        CHECK_EQ(e.requested(), 9);
    }
}

TEST(cartitem_equality_and_lines) {
    auto a = std::make_shared<Electronics>(1, "A", 1000.0, 5, 6);
    auto b = std::make_shared<Electronics>(2, "B", 1000.0, 5, 6);
    CartItem x(a, 1);
    CartItem y(a, 3);
    CartItem z(b, 1);
    CHECK(x == y);   // same product, different quantity
    CHECK(x != z);
    CHECK_EQ(y.lineMrp(), rs("3000"));
    CHECK_EQ(y.lineDiscount(), rs("300"));
    CHECK_EQ(y.lineTotal(), rs("2700"));
    CartItem copy(y);   // copy constructor shares the product
    CHECK_EQ(copy.quantity(), 3);
    CHECK_EQ(a.use_count(), 4L);   // a + x + y + copy
}

TEST(price_breakdown_rules) {
    auto rice = std::make_shared<Grocery>(1, "Rice", 100.0, 10, "2027-01-01");
    Cart small;
    small.addItem(rice, 2);
    PriceBreakdown b = small.breakdown();
    CHECK_EQ(b.mrpTotal, rs("200"));
    CHECK_EQ(b.productDiscount, rs("40"));
    CHECK_EQ(b.subtotal, rs("160"));
    CHECK_EQ(b.deliveryFee, rs("49"));   // below Rs 500
    CHECK_EQ(b.gst, rs("8"));            // 5% of 160
    CHECK_EQ(b.total, rs("217"));

    auto speaker = std::make_shared<Electronics>(2, "Speaker", 1000.0, 10, 12);
    Cart big;
    big.addItem(speaker, 1);
    b = big.breakdown();
    CHECK_EQ(b.subtotal, rs("900"));
    CHECK(b.deliveryFee.isZero());
    CHECK_EQ(b.gst, rs("45"));
    CHECK_EQ(b.total, rs("945"));

    big.applyCoupon("save10");           // case-insensitive
    b = big.breakdown();
    CHECK_EQ(b.couponDiscount, rs("90"));
    CHECK_EQ(b.gst, rs("40.50"));        // GST after the coupon
    CHECK_EQ(b.total, rs("850.50"));
    CHECK_EQ(b.totalSavings(), rs("190"));

    Cart empty;
    CHECK(empty.breakdown().total.isZero());
}

// ---------------------------------------------------------------------------
// Coupons
// ---------------------------------------------------------------------------

TEST(coupon_rules) {
    auto save10 = CouponBook::find("SAVE10");
    auto flat = CouponBook::find("flat200");
    auto festive = CouponBook::find("FESTIVE25");
    CHECK_EQ(save10->discountFor(rs("2000")), rs("200"));
    CHECK_EQ(save10->discountFor(rs("20000")), rs("1000"));    // capped
    CHECK_EQ(flat->discountFor(rs("999")), Money());            // below minimum
    CHECK_EQ(flat->discountFor(rs("1500")), rs("200"));
    CHECK_EQ(festive->discountFor(rs("6000")), rs("1500"));
    CHECK_EQ(festive->discountFor(rs("20766.35")), rs("2500")); // capped
    CHECK(!festive->isApplicable(rs("4999.99")));
    CHECK(!festive->whyNot(rs("4000")).empty());
    CHECK_EQ(save10->kind(), std::string("Percent"));
    CHECK_EQ(flat->kind(), std::string("Flat"));
    CHECK_THROWS(CouponBook::find("MEGA50"), CouponException);
    CHECK_EQ(CouponBook::all().size(), static_cast<std::size_t>(3));
}

TEST(coupon_apply_to_cart) {
    auto tee = std::make_shared<Clothing>(1, "Tee", 500.0, 10, "M");
    Cart cart;
    cart.addItem(tee, 1);   // subtotal 475
    CHECK_THROWS(cart.applyCoupon("FLAT200"), CouponException);   // min order 1000
    CHECK(!cart.hasCoupon());
    cart.addItem(tee, 2);   // subtotal 1425
    cart.applyCoupon("FLAT200");
    CHECK(cart.hasCoupon());
    CHECK_EQ(cart.breakdown().couponDiscount, rs("200"));
    cart.updateQuantity(1, 1);   // drops below the minimum again
    CHECK(cart.breakdown().couponDiscount.isZero());
    CHECK(!cart.breakdown().couponNote.empty());
    cart.removeCoupon();
    CHECK(!cart.hasCoupon());
}

// ---------------------------------------------------------------------------
// Payments
// ---------------------------------------------------------------------------

TEST(upi_payment) {
    UpiPayment ok(rs("500"), "tanaya@okaxis");
    std::string ref = ok.pay();
    CHECK(util::startsWith(ref, "UPI-"));
    CHECK(ok.isPaid());
    CHECK_THROWS(ok.pay(), PaymentException);   // cannot pay twice
    UpiPayment bad(rs("500"), "tanaya.okaxis");
    CHECK_THROWS(bad.pay(), PaymentException);
    CHECK(!bad.isPaid());
    UpiPayment tooBig(rs("150000"), "tanaya@okaxis");
    CHECK_THROWS(tooBig.pay(), PaymentException);
    CHECK(UpiPayment::isValidUpiId("ab@ybl"));
    CHECK(!UpiPayment::isValidUpiId("a@ybl"));
    CHECK(!UpiPayment::isValidUpiId("abc@y1"));
}

TEST(card_payment) {
    util::setFixedNow("2026-11-20 10:30");
    CHECK(CardPayment::passesLuhn("4111111111111111"));
    CHECK(!CardPayment::passesLuhn("4111111111111112"));
    CardPayment ok(rs("999"), "4111 1111 1111 1111", "Tanaya", "08/29", "123");
    CHECK(util::startsWith(ok.pay(), "CARD-"));
    CHECK_EQ(ok.details(), std::string("VISA **** 1111"));
    CardPayment declined(rs("999"), CardPayment::declinedTestCard(), "Tanaya", "08/29", "123");
    CHECK_THROWS(declined.pay(), PaymentException);
    CardPayment expired(rs("999"), "4111111111111111", "Tanaya", "10/26", "123");
    CHECK_THROWS(expired.pay(), PaymentException);
    CardPayment thisMonth(rs("999"), "4111111111111111", "Tanaya", "11/26", "123");
    CHECK(!thisMonth.pay().empty());
    CardPayment badCvv(rs("999"), "4111111111111111", "Tanaya", "08/29", "12");
    CHECK_THROWS(badCvv.pay(), PaymentException);
    CardPayment letters(rs("999"), "4111-1111-abcd-1111", "Tanaya", "08/29", "123");
    CHECK_THROWS(letters.pay(), PaymentException);
    util::clearFixedNow();
}

TEST(cod_and_wallet_payment) {
    CashOnDelivery cod(rs("2000"));
    CHECK(!cod.isPrepaid());
    CHECK(util::startsWith(cod.pay(), "COD-"));
    CashOnDelivery codTooBig(rs("60000"));
    CHECK_THROWS(codTooBig.pay(), PaymentException);

    Customer c(10, "Test User", "t@x.com", "9999999999", "Somewhere 1", "", rs("1000"), "2026-01-01");
    WalletPayment tooMuch(rs("1500"), c);
    CHECK_THROWS(tooMuch.pay(), PaymentException);
    CHECK_EQ(c.getWallet(), rs("1000"));   // untouched on failure
    WalletPayment fine(rs("400"), c);
    CHECK(util::startsWith(fine.pay(), "WAL-"));
    CHECK_EQ(c.getWallet(), rs("600"));
}

TEST(payment_polymorphism) {
    Customer c(11, "Test User", "t@x.com", "9999999999", "Somewhere 1", "", rs("5000"), "2026-01-01");
    std::vector<std::unique_ptr<Payment>> methods;
    methods.push_back(std::unique_ptr<Payment>(new UpiPayment(rs("100"), "abc@okhdfc")));
    methods.push_back(std::unique_ptr<Payment>(new CashOnDelivery(rs("100"))));
    methods.push_back(std::unique_ptr<Payment>(new WalletPayment(rs("100"), c)));
    std::vector<std::string> names;
    for (auto& m : methods) {
        names.push_back(m->methodName());
        CHECK(!m->processingSteps().empty());
        m->pay();
        CHECK(m->isPaid());
    }
    CHECK_EQ(names[0], std::string("UPI"));
    CHECK_EQ(names[1], std::string("Cash on Delivery"));
    CHECK_EQ(names[2], std::string("ShopSphere Wallet"));
}

// ---------------------------------------------------------------------------
// Exceptions
// ---------------------------------------------------------------------------

TEST(exception_hierarchy) {
    try {
        throw OutOfStockException("Laptop", 5, 2);
    } catch (const ShopException& e) {   // caught through the base class
        CHECK_EQ(e.title(), std::string("Out of stock"));
        CHECK_EQ(e.code(), std::string("STOCK"));
        CHECK(std::string(e.what()).find("Laptop") != std::string::npos);
        CHECK(std::string(e.what()).find("2") != std::string::npos);
    }
    try {
        throw PaymentException("declined");
    } catch (const std::runtime_error& e) {   // ShopException IS-A std::runtime_error
        CHECK_EQ(std::string(e.what()), std::string("declined"));
    }
    AuthException a("x");
    CouponException c("x");
    NotFoundException n("x");
    FileException f("x");
    InvalidInputException i("x");
    const ShopException* all[5] = {&a, &c, &n, &f, &i};
    std::vector<std::string> titles;
    for (const ShopException* e : all) titles.push_back(e->title());
    CHECK_EQ(titles[0], std::string("Authentication failed"));
    CHECK_EQ(titles[1], std::string("Coupon not applied"));
    CHECK_EQ(titles[2], std::string("Not found"));
    CHECK_EQ(titles[3], std::string("File error"));
    CHECK_EQ(titles[4], std::string("Invalid input"));
}

// ---------------------------------------------------------------------------
// Repository<T> template
// ---------------------------------------------------------------------------

TEST(repository_template) {
    Repository<Product> repo;
    repo.add(std::make_shared<Electronics>(3, "Charlie", 300.0, 1, 6));
    repo.add(std::make_shared<Grocery>(1, "Alpha", 100.0, 9, "2027-01-01"));
    repo.add(std::make_shared<Clothing>(2, "Bravo", 200.0, 4, "M"));
    CHECK_EQ(repo.size(), static_cast<std::size_t>(3));
    CHECK_THROWS(repo.add(std::make_shared<Grocery>(1, "Dup", 1.0, 1, "x")), InvalidInputException);
    CHECK(repo.findById(2) != nullptr);
    CHECK(repo.findById(42) == nullptr);
    CHECK_THROWS(repo.getById(42), NotFoundException);
    CHECK_EQ(repo.maxId(), 3);

    auto cheap = repo.findAll([](const Product& p) { return p.getPrice() < 250; });
    CHECK_EQ(cheap.size(), static_cast<std::size_t>(2));
    CHECK_EQ(repo.countIf([](const Product& p) { return p.getStock() > 3; }), static_cast<std::size_t>(2));
    auto byName = repo.sortedBy([](const Product& p) { return p.getName(); });
    CHECK_EQ(byName.front()->getName(), std::string("Alpha"));
    auto byPriceDesc = repo.sortedBy([](const Product& p) { return p.getPrice(); }, false);
    CHECK_EQ(byPriceDesc.front()->getId(), 3);
    auto first = repo.findFirst([](const Product& p) { return p.getCategory() == "Clothing"; });
    CHECK(first && first->getId() == 2);

    std::vector<ProductPtr> list = repo.all();
    sortBy(list, [](const ProductPtr& p) { return p->getId(); });   // free function template
    CHECK_EQ(list.front()->getId(), 1);
    CHECK_EQ(findById(list, 3)->getName(), std::string("Charlie"));
    CHECK(findById(list, 99) == nullptr);

    CHECK(repo.remove(1));
    CHECK(!repo.remove(1));
    CHECK_EQ(repo.size(), static_cast<std::size_t>(2));
}

// ---------------------------------------------------------------------------
// Orders & store
// ---------------------------------------------------------------------------

TEST(order_totals_match_cart) {
    auto phone = std::make_shared<Smartphone>(1, "Phone", 17999.0, 5, 12, 128);
    auto rice = std::make_shared<Grocery>(2, "Rice", 649.0, 50, "2027-01-01");
    Cart cart;
    cart.addItem(phone, 1);
    cart.addItem(rice, 2);
    cart.applyCoupon("FESTIVE25");
    Customer c(5, "Buyer", "b@x.com", "9999999999", "Pune 411001");
    OrderPtr o = Order::fromCart(cart, c, c.getAddress(), "2026-11-20 10:30");
    CHECK_EQ(o->total(), cart.total());
    CHECK_EQ(o->units(), 3);
    CHECK_EQ(o->getCouponCode(), std::string("FESTIVE25"));
    CHECK_EQ(o->totals().couponDiscount, rs("2500"));
    CHECK(o->getStatus() == OrderStatus::Placed);
    o->cancel();
    CHECK(o->getStatus() == OrderStatus::Cancelled);
    CHECK_THROWS(o->cancel(), InvalidInputException);
    o->setStatus(OrderStatus::Delivered);
    CHECK(!o->isCancellable());
    CHECK(statusFromName(statusName(OrderStatus::Shipped)) == OrderStatus::Shipped);
    std::ostringstream os;
    os << *o;
    CHECK(os.str().find("Order #") != std::string::npos);
    Cart empty;
    CHECK_THROWS(Order::fromCart(empty, c, "x", "y"), InvalidInputException);
}

TEST(store_checkout_cancel_and_report) {
    Store store("test_tmp_unused", "test_tmp_invoices");
    store.loadSeed();
    store.setPersistence(false);
    CHECK_EQ(store.products().size(), static_cast<std::size_t>(23));
    CHECK_EQ(store.customers().size(), static_cast<std::size_t>(4));
    CHECK_EQ(store.orders().size(), static_cast<std::size_t>(8));

    SalesReport seedReport = store.salesReport();
    CHECK_EQ(seedReport.orders, 7);
    CHECK_EQ(seedReport.cancelled, 1);
    CHECK_EQ(seedReport.units, 31);
    CHECK_EQ(seedReport.revenue, rs("95378.53"));
    CHECK_EQ(seedReport.revenueByCategory.size(), static_cast<std::size_t>(3));

    std::shared_ptr<Customer> tanaya = store.auth().login("tanaya@shop.com", "pass123");
    Cart cart;
    store.addToCart(cart, 17, 2);   // rice, stock 50
    store.addToCart(cart, 20, 1);   // butter, stock 35
    CHECK_THROWS(store.addToCart(cart, 999, 1), NotFoundException);
    CHECK_THROWS(store.addToCart(cart, 14, 3), OutOfStockException);   // hoodie, stock 2

    std::shared_ptr<Customer> guest = AuthService::guest();
    UpiPayment guestPay(cart.total(), "guest@okaxis");
    CHECK_THROWS(store.placeOrder(*guest, cart, guestPay, "Somewhere"), AuthException);

    UpiPayment wrongAmount(rs("1"), "tanaya@okaxis");
    CHECK_THROWS(store.placeOrder(*tanaya, cart, wrongAmount, tanaya->getAddress()), PaymentException);

    Money total = cart.total();
    UpiPayment pay(total, "tanaya@okaxis");
    CheckoutResult r = store.placeOrder(*tanaya, cart, pay, tanaya->getAddress());
    CHECK(r.order != nullptr);
    CHECK(cart.isEmpty());
    CHECK_EQ(store.product(17)->getStock(), 48);
    CHECK_EQ(store.product(20)->getStock(), 34);
    CHECK_EQ(store.orders().size(), static_cast<std::size_t>(9));
    CHECK_EQ(store.ordersOf(tanaya->getId()).front()->getId(), r.order->getId());   // newest first
    CHECK(!r.invoicePath.empty());
    CHECK(FileStore::fileExists(r.invoicePath));

    Money walletBefore = tanaya->getWallet();
    Money refund = store.cancelOrder(r.order->getId(), tanaya->getId(), false);
    CHECK_EQ(refund, total);                                 // prepaid -> refunded to wallet
    CHECK_EQ(tanaya->getWallet(), walletBefore + total);
    CHECK_EQ(store.product(17)->getStock(), 50);             // restocked
    CHECK_THROWS(store.cancelOrder(r.order->getId(), tanaya->getId(), false), InvalidInputException);
    int viralsOrder = store.ordersOf(2).front()->getId();
    CHECK_THROWS(store.cancelOrder(viralsOrder, tanaya->getId(), false), NotFoundException);   // not hers

    CHECK_EQ(store.salesReport().cancelled, 2);
    CHECK_EQ(store.lowStock().front()->getStock(), 2);       // hoodie is the lowest

    ProductPtr added = store.addProduct("Grocery", "Masala Chai 250g", 180.0, 3, {"2027-05-01"}, 4.2);
    CHECK_EQ(store.products().size(), static_cast<std::size_t>(24));
    store.restock(added->getId(), 7);
    CHECK_EQ(added->getStock(), 10);
    store.removeProduct(added->getId());
    CHECK_THROWS(store.product(added->getId()), NotFoundException);

    std::remove(r.invoicePath.c_str());
    SHOP_RMDIR("test_tmp_invoices");
}

TEST(auth_rules) {
    Repository<Customer> customers;
    AuthService auth(customers);
    auto c = auth.registerCustomer("Riya Sharma", "Riya@Example.com", "9876543210", "secret1",
                                   "Baner, Pune 411045");
    CHECK_EQ(c->getEmail(), std::string("riya@example.com"));
    CHECK_EQ(c->getWallet(), AuthService::welcomeBonus());
    CHECK(auth.login("RIYA@example.com", "secret1") == c);
    CHECK_THROWS(auth.login("riya@example.com", "wrong1"), AuthException);
    CHECK_THROWS(auth.login("nobody@example.com", "secret1"), AuthException);
    CHECK_THROWS(auth.registerCustomer("Riya Two", "riya@example.com", "9876543210", "secret1", "Pune 1"),
                 InvalidInputException);   // duplicate email
    CHECK_THROWS(auth.registerCustomer("Bad Mail", "bad.mail", "9876543210", "secret1", "Pune 1"),
                 InvalidInputException);
    CHECK_THROWS(auth.registerCustomer("Bad Phone", "p@x.com", "12345", "secret1", "Pune 1"),
                 InvalidInputException);
    CHECK_THROWS(auth.registerCustomer("Weak Pass", "w@x.com", "9876543210", "abcdef", "Pune 1"),
                 InvalidInputException);
    CHECK_THROWS(auth.registerCustomer("X", "x@x.com", "9876543210", "secret1", "Pune 1"),
                 InvalidInputException);
    CHECK_EQ(auth.adminLogin("admin", "admin123").getRole(), std::string("Administrator"));
    CHECK_THROWS(auth.adminLogin("admin", "admin"), AuthException);
    CHECK(AuthService::guest()->isGuest());
}

TEST(customer_encapsulation) {
    Customer c(101, "Riya Sharma", "riya.sharma@example.com", "9876543210", "Pune");
    CHECK(!c.setEmail("not-an-email"));
    CHECK_EQ(c.getEmail(), std::string("riya.sharma@example.com"));
    CHECK(c.setEmail("riya@new.com"));
    CHECK(!c.setPhone("12345"));
    CHECK(!c.setPhone("98765abcde"));
    CHECK(c.setPhone("9123456780"));
    CHECK_EQ(c.getRole(), std::string("Customer"));
    CHECK_EQ(c.firstName(), std::string("Riya"));
    c.setPassword("hello123");
    CHECK(c.checkPassword("hello123"));
    CHECK(!c.checkPassword("hello124"));
    CHECK_THROWS(c.debitWallet(rs("1")), PaymentException);
    CHECK_THROWS(c.creditWallet(rs("-5")), InvalidInputException);
}

// ---------------------------------------------------------------------------
// Catalog queries
// ---------------------------------------------------------------------------

TEST(catalog_filter_search_sort) {
    Catalog catalog(seed::products());
    CHECK_EQ(catalog.size(), static_cast<std::size_t>(23));
    CHECK_EQ(catalog.inCategory("Electronics").size(), static_cast<std::size_t>(8));
    CHECK_EQ(catalog.inCategory("Clothing").size(), static_cast<std::size_t>(8));
    CHECK_EQ(catalog.inCategory("Grocery").size(), static_cast<std::size_t>(7));
    CHECK_EQ(catalog.inCategory("All").size(), static_cast<std::size_t>(23));
    CHECK_EQ(catalog.matching("SHIRT").size(), static_cast<std::size_t>(2));
    CHECK_EQ(catalog.matching("smartphone").size(), static_cast<std::size_t>(2));   // by type
    Catalog cheapFirst = catalog.sortedBy(SortKey::PriceLowHigh);
    CHECK_EQ(cheapFirst.items().front()->getName(), std::string("Parle-G Biscuits 250g"));
    Catalog deals = catalog.inCategory("Electronics").sortedBy(SortKey::BiggestDiscount);
    CHECK_EQ(deals.items().front()->getType(), std::string("Smartphone"));
    Catalog rated = catalog.sortedBy(SortKey::Rating);
    CHECK_EQ(rated.items().front()->getName(), std::string("Banarasi Silk Saree"));
    CHECK(Catalog::nextSort(SortKey::Name) == SortKey::Featured);
}

// ---------------------------------------------------------------------------
// File handling
// ---------------------------------------------------------------------------

TEST(csv_parsing) {
    auto f = FileStore::parseCsvLine("1,\"Pune, MH\",\"say \"\"hi\"\"\",,end\r");
    CHECK_EQ(f.size(), static_cast<std::size_t>(5));
    CHECK_EQ(f[1], std::string("Pune, MH"));
    CHECK_EQ(f[2], std::string("say \"hi\""));
    CHECK_EQ(f[3], std::string(""));
    CHECK_EQ(f[4], std::string("end"));
    std::string line = FileStore::toCsvLine({"a", "b,c", "q\"x"});
    CHECK_EQ(line, std::string("a,\"b,c\",\"q\"\"x\""));
    CHECK_EQ(FileStore::parseCsvLine(line)[2], std::string("q\"x"));
}

TEST(file_round_trip) {
    const std::string dir = "test_tmp_data";
    CHECK(FileStore::ensureDirectory(dir));
    std::vector<ProductPtr> products = seed::products();
    std::vector<std::shared_ptr<Customer>> customers = seed::customers();
    std::vector<OrderPtr> orders = seed::orders(products, customers);

    FileStore files(dir);
    files.saveProducts(products);
    files.saveCustomers(customers);
    files.saveOrders(orders);
    CHECK(files.hasData());

    std::vector<ProductPtr> p2 = files.loadProducts();
    std::vector<std::shared_ptr<Customer>> c2 = files.loadCustomers();
    std::vector<OrderPtr> o2 = files.loadOrders();
    CHECK_EQ(files.skippedRows(), 0);
    CHECK_EQ(p2.size(), products.size());
    CHECK_EQ(c2.size(), customers.size());
    CHECK_EQ(o2.size(), orders.size());
    if (p2.size() == products.size()) {
        for (std::size_t i = 0; i < p2.size(); ++i) {
            CHECK_EQ(p2[i]->getType(), products[i]->getType());
            CHECK_EQ(p2[i]->getName(), products[i]->getName());
            CHECK_EQ(Money(p2[i]->getFinalPrice()), Money(products[i]->getFinalPrice()));
            CHECK_EQ(p2[i]->attributeSummary(), products[i]->attributeSummary());
        }
    }
    if (c2.size() == customers.size()) {
        CHECK_EQ(c2[0]->getAddress(), customers[0]->getAddress());   // contains a comma
        CHECK_EQ(c2[0]->getWallet(), customers[0]->getWallet());
        CHECK(c2[0]->checkPassword("pass123"));
    }
    if (o2.size() == orders.size()) {
        for (std::size_t i = 0; i < o2.size(); ++i) {
            CHECK_EQ(o2[i]->total(), orders[i]->total());
            CHECK(o2[i]->getStatus() == orders[i]->getStatus());
            CHECK_EQ(o2[i]->getLines().size(), orders[i]->getLines().size());
        }
    }

    // The committed data/ files must match the built-in seed exactly.
    FileStore shipped("data");
    if (shipped.hasData()) {
        std::vector<OrderPtr> o3 = shipped.loadOrders();
        CHECK_EQ(o3.size(), orders.size());
        for (std::size_t i = 0; i < o3.size() && i < orders.size(); ++i) {
            CHECK_EQ(o3[i]->total(), orders[i]->total());
        }
        CHECK_EQ(shipped.loadProducts().size(), products.size());
    }

    // A malformed row is skipped, not fatal.
    {
        std::ofstream out(FileStore::join(dir, "products.csv").c_str(), std::ios::app);
        out << "oops,Furniture,Chair\n";
    }
    CHECK_EQ(files.loadProducts().size(), products.size());
    CHECK_EQ(files.skippedRows(), 1);

    const char* names[4] = {"products.csv", "customers.csv", "orders.csv", "order_items.csv"};
    for (const char* n : names) std::remove(FileStore::join(dir, n).c_str());
    SHOP_RMDIR(dir.c_str());
}

TEST(invoice_rendering) {
    std::vector<ProductPtr> products = seed::products();
    std::vector<std::shared_ptr<Customer>> customers = seed::customers();
    std::vector<OrderPtr> orders = seed::orders(products, customers);
    InvoiceWriter writer("invoices");
    std::string text = writer.render(*orders[0], customers[0].get());
    CHECK(text.find("INV-" + util::toStr(orders[0]->getId())) != std::string::npos);
    CHECK(text.find("GRAND TOTAL") != std::string::npos);
    CHECK(text.find(orders[0]->total().plain()) != std::string::npos);
    CHECK(text.find("SAVE10") != std::string::npos);
    CHECK(!text.empty() && text[text.size() - 1] == '\n');
}

// ---------------------------------------------------------------------------
// String utilities
// ---------------------------------------------------------------------------

TEST(string_utilities) {
    CHECK_EQ(util::displayWidth("\x1b[38;2;1;2;3mabc\x1b[0m"), 3);
    CHECK_EQ(util::displayWidth("₹1,499 ★"), 8);
    CHECK_EQ(util::padRight("ab", 4), std::string("ab  "));
    CHECK_EQ(util::padLeft("ab", 4), std::string("  ab"));
    CHECK_EQ(util::displayWidth(util::truncate("abcdefghij", 5)), 5);
    CHECK_EQ(util::trim("  hi \t"), std::string("hi"));
    CHECK_EQ(util::split("a,b,,c", ',').size(), static_cast<std::size_t>(4));
    CHECK_EQ(util::hashPassword("pass123"), std::string("f33d47656d014ea2"));
    CHECK(util::isValidEmail("a@b.co"));
    CHECK(!util::isValidEmail("a@b"));
    CHECK(!util::isValidEmail("@b.com"));
    int v = 0;
    CHECK(util::parseInt(" 42 ", v) && v == 42);
    CHECK(!util::parseInt("4x2", v));
    CHECK_EQ(util::toStr(3.5), std::string("3.5"));
    util::setFixedNow("2026-11-20 10:30");
    CHECK_EQ(util::currentYear(), 2026);
    CHECK_EQ(util::currentMonth(), 11);
    util::clearFixedNow();
}

// ---------------------------------------------------------------------------

int main() {
    Money::setSymbol("Rs ");
    int failedTests = 0;
    std::cout << "ShopSphere test suite\n=====================\n";
    for (const TestCase& t : registry()) {
        g_currentFailed = false;
        try {
            t.body();
        } catch (const std::exception& e) {
            g_currentFailed = true;
            std::cout << "      unexpected exception: " << e.what() << "\n";
        }
        std::cout << (g_currentFailed ? "  [FAIL] " : "  [PASS] ") << t.name << "\n";
        if (g_currentFailed) ++failedTests;
    }
    std::cout << "---------------------\n"
              << registry().size() - static_cast<std::size_t>(failedTests) << "/" << registry().size()
              << " tests passed, " << g_checks - g_failedChecks << "/" << g_checks << " checks passed\n";
    return failedTests == 0 ? 0 : 1;
}
