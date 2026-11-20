// Part B demo driver: CartItem / Cart / Order / Repository<T>, operator
// overloading, STL algorithms and static members. Prints a readable trace.

#include <iostream>

#include "cart/Cart.h"
#include "cart/Order.h"
#include "cart/Repository.h"
#include "core/Catalog.h"
#include "core/Customer.h"
#include "services/SeedData.h"

using namespace shop;

static void printDivider(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

int main() {
    printDivider("Repository<Product> (template) loaded with the seed catalog");
    Repository<Product> products;
    for (const ProductPtr& p : seed::products()) products.add(p);
    std::cout << products.size() << " products, highest id " << products.maxId() << "\n";

    auto cheapest = products.sortedBy([](const Product& p) { return p.getFinalPrice(); });
    std::cout << "Cheapest: " << *cheapest.front() << "\n";   // friend operator<<
    std::cout << "Dearest : " << *cheapest.back() << "\n";

    printDivider("Cart += / -= (operator overloading)");
    Cart cart;
    cart += CartItem(products.getById(2), 1);    // Redmi Note 13 5G
    cart += CartItem(products.getById(17), 2);   // Basmati rice
    cart += CartItem(products.getById(17), 1);   // merges: CartItem::operator==
    cart += CartItem(products.getById(10), 2);   // T-shirt
    for (const CartItem& item : cart.items()) std::cout << "  " << item << "\n";
    cart -= 10;
    std::cout << "After cart -= 10: " << cart.lineCount() << " lines, " << cart.units() << " units\n";

    printDivider("Price breakdown with coupon FESTIVE25");
    cart.applyCoupon("FESTIVE25");
    PriceBreakdown b = cart.breakdown();
    std::cout << "MRP total        " << b.mrpTotal << "\n"
              << "Product discount -" << b.productDiscount << "\n"
              << "Subtotal         " << b.subtotal << "\n"
              << "Coupon           -" << b.couponDiscount << "\n"
              << "Delivery         " << b.deliveryFee << "\n"
              << "GST 5%           " << b.gst << "\n"
              << "Grand total      " << b.total << "\n";

    printDivider("Order snapshot + static order counter");
    Customer buyer(1, "Tanaya Thorat", "tanaya@shop.com", "9876543210", "Bibwewadi, Pune");
    OrderPtr order = Order::fromCart(cart, buyer, buyer.getAddress(), "2026-11-20 10:30");
    order->setPayment("UPI", "UPI-000001");
    std::cout << *order << "\n";
    std::cout << "Next order id would be " << Order::generateId() << "\n";

    printDivider("Out-of-stock is an exception, not a silent failure");
    try {
        cart += CartItem(products.getById(14), 5);   // hoodie, only 2 in stock
    } catch (const ShopException& e) {
        std::cout << e.title() << ": " << e.what() << "\n";
    }
    return 0;
}
