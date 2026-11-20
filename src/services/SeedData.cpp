#include "services/SeedData.h"

#include "cart/Cart.h"
#include "cart/Repository.h"
#include "core/Clothing.h"
#include "core/Electronics.h"
#include "core/Grocery.h"
#include "core/Smartphone.h"
#include "util/StringUtil.h"

namespace shop {
namespace seed {

namespace {
ProductPtr rated(Product* p, double rating) {
    ProductPtr sp(p);
    sp->setRating(rating);
    return sp;
}
}  // namespace

std::vector<ProductPtr> products() {
    return {
        // Electronics (warranty in months) + Smartphones (warranty, storage GB)
        rated(new Electronics(1, "boAt Airdopes 141 Earbuds", 1499.0, 40, 12), 4.1),
        rated(new Smartphone(2, "Redmi Note 13 5G", 17999.0, 15, 12, 128), 4.3),
        rated(new Smartphone(3, "Samsung Galaxy M34 5G", 16499.0, 3, 12, 128), 4.2),
        rated(new Electronics(4, "HP Pavilion 14 Laptop", 62999.0, 6, 24), 4.4),
        rated(new Electronics(5, "Noise ColorFit Pro 5 Watch", 3499.0, 25, 12), 4.0),
        rated(new Electronics(6, "JBL Go 3 Bluetooth Speaker", 2999.0, 18, 12), 4.5),
        rated(new Electronics(7, "Logitech MK270 Keyboard Combo", 1795.0, 30, 36), 4.4),
        rated(new Electronics(8, "Mi 20000mAh Power Bank", 1999.0, 4, 6), 4.3),
        // Clothing (size)
        rated(new Clothing(9, "Levi's 511 Slim Fit Jeans", 2799.0, 20, "32"), 4.3),
        rated(new Clothing(10, "Cotton Crew Neck T-Shirt", 499.0, 60, "M"), 4.0),
        rated(new Clothing(11, "Fabindia Cotton Kurta", 1299.0, 14, "L"), 4.6),
        rated(new Clothing(12, "Puma Running Shoes", 3999.0, 12, "UK 9"), 4.4),
        rated(new Clothing(13, "Allen Solly Formal Shirt", 1499.0, 25, "L"), 4.2),
        rated(new Clothing(14, "Fleece Winter Hoodie", 1899.0, 2, "XL"), 4.1),
        rated(new Clothing(15, "Banarasi Silk Saree", 4599.0, 8, "Free"), 4.8),
        rated(new Clothing(16, "Ankle Socks (Pack of 3)", 299.0, 80, "Free"), 3.9),
        // Grocery (best-before date)
        rated(new Grocery(17, "India Gate Basmati Rice 5kg", 649.0, 50, "2027-06-30"), 4.5),
        rated(new Grocery(18, "Fortune Sunflower Oil 1L", 155.0, 70, "2027-03-15"), 4.2),
        rated(new Grocery(19, "Tata Sampann Toor Dal 1kg", 179.0, 60, "2027-04-20"), 4.4),
        rated(new Grocery(20, "Amul Butter 500g", 285.0, 35, "2026-12-28"), 4.7),
        rated(new Grocery(21, "Aashirvaad Atta 10kg", 489.0, 40, "2027-02-10"), 4.5),
        rated(new Grocery(22, "Tata Tea Gold 500g", 310.0, 45, "2027-08-01"), 4.3),
        rated(new Grocery(23, "Parle-G Biscuits 250g", 25.0, 200, "2027-01-15"), 4.6),
    };
}

std::vector<std::shared_ptr<Customer>> customers() {
    const std::string pw = util::hashPassword("pass123");
    return {
        std::make_shared<Customer>(1, "Tanaya Thorat", "tanaya@shop.com", "9876543210",
                                   "Flat 12, Bibwewadi, Pune 411037", pw, Money(2500.0),
                                   "2026-08-14"),
        std::make_shared<Customer>(2, "Viral Dhoka", "viral@shop.com", "9822012345",
                                   "Kothrud, Pune 411038", pw, Money(1500.0), "2026-08-20"),
        std::make_shared<Customer>(3, "Shreenath Utekar", "shreenath@shop.com", "9763054321",
                                   "Upper Indiranagar, Pune 411037", pw, Money(1000.0),
                                   "2026-09-02"),
        std::make_shared<Customer>(4, "Harshali Thakur", "harshali@shop.com", "9890123456",
                                   "Katraj, Pune 411046", pw, Money(3000.0), "2026-09-11"),
    };
}

namespace {
struct SeedOrder {
    int customerIndex;
    std::string date;
    OrderStatus status;
    std::string method;
    std::string ref;
    std::string coupon;
    std::vector<std::pair<int, int>> items;   // productId, qty
};
}  // namespace

std::vector<OrderPtr> orders(const std::vector<ProductPtr>& products,
                             const std::vector<std::shared_ptr<Customer>>& customers) {
    const std::vector<SeedOrder> plan = {
        {0, "2026-11-02 11:24", OrderStatus::Delivered, "UPI", "UPI-480101", "SAVE10",
         {{2, 1}, {1, 1}}},
        {1, "2026-11-05 19:02", OrderStatus::Delivered, "Card", "CARD-480102", "FESTIVE25",
         {{4, 1}}},
        {2, "2026-11-08 09:40", OrderStatus::Delivered, "Cash on Delivery", "COD-480103", "",
         {{17, 2}, {19, 3}, {20, 2}, {21, 1}}},
        {3, "2026-11-10 16:15", OrderStatus::Shipped, "ShopSphere Wallet", "WAL-480104", "FLAT200",
         {{15, 1}, {11, 1}}},
        {0, "2026-11-12 13:05", OrderStatus::Delivered, "Card", "CARD-480105", "",
         {{9, 1}, {12, 1}, {16, 2}}},
        {1, "2026-11-14 21:30", OrderStatus::Cancelled, "UPI", "UPI-480106", "", {{6, 1}}},
        {2, "2026-11-16 10:12", OrderStatus::Shipped, "UPI", "UPI-480107", "SAVE10",
         {{5, 1}, {7, 1}, {8, 1}}},
        {3, "2026-11-18 18:45", OrderStatus::Placed, "Cash on Delivery", "COD-480108", "",
         {{22, 2}, {18, 2}, {23, 5}, {10, 2}}},
    };

    std::vector<OrderPtr> out;
    for (const SeedOrder& s : plan) {
        const auto& customer = customers.at(static_cast<std::size_t>(s.customerIndex));
        Cart cart;
        for (const auto& it : s.items) {
            ProductPtr p = findById(products, it.first);   // OOP: templates -- generic findById
            if (p) cart.addItem(p, it.second);
        }
        if (!s.coupon.empty()) cart.applyCoupon(s.coupon);
        OrderPtr o = Order::fromCart(cart, *customer, customer->getAddress(), s.date);
        o->setStatus(s.status);
        o->setPayment(s.method, s.ref);
        out.push_back(o);
    }
    return out;
}

}  // namespace seed
}  // namespace shop
