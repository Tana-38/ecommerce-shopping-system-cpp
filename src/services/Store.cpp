#include "services/Store.h"

#include <algorithm>
#include <numeric>

#include "services/SeedData.h"
#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

Store::Store(const std::string& dataDir, const std::string& invoiceDir)
    : files_(new FileStore(dataDir)), auth_(new AuthService(customers_)),
      invoices_(invoiceDir), persist_(true) {}

Store::~Store() {}

void Store::load() {
    if (!files_->hasData()) {
        loadSeed();
        autosave();   // first run: write the seed files
        return;
    }
    products_.clear();
    customers_.clear();
    orders_.clear();
    // Duplicate IDs in hand-edited files are skipped instead of aborting the load.
    for (const ProductPtr& p : files_->loadProducts()) {
        if (!products_.findById(p->getId())) products_.add(p);
    }
    for (const auto& c : files_->loadCustomers()) {
        if (!customers_.findById(c->getId())) customers_.add(c);
    }
    for (const OrderPtr& o : files_->loadOrders()) {
        if (!orders_.findById(o->getId())) orders_.add(o);
    }
}

void Store::loadSeed() {
    products_.clear();
    customers_.clear();
    orders_.clear();
    std::vector<ProductPtr> ps = seed::products();
    std::vector<std::shared_ptr<Customer>> cs = seed::customers();
    for (const ProductPtr& p : ps) products_.add(p);
    for (const auto& c : cs) customers_.add(c);
    for (const OrderPtr& o : seed::orders(ps, cs)) orders_.add(o);
}

void Store::save() const {
    files_->saveProducts(products_.all());
    files_->saveCustomers(customers_.all());
    files_->saveOrders(orders_.all());
}

void Store::autosave() const {
    if (persist_) save();
}

void Store::saveCustomers() const {
    if (persist_) files_->saveCustomers(customers_.all());
}

ProductPtr Store::product(int id) const {
    ProductPtr p = products_.findById(id);
    if (!p) throw NotFoundException("No product with ID " + util::toStr(id) + ".");
    return p;
}

void Store::addToCart(Cart& cart, int productId, int qty) const {
    ProductPtr p = product(productId);
    if (p->getStock() == 0) throw OutOfStockException(p->getName(), qty, 0);
    cart.addItem(p, qty);   // validates qty against stock (merging with existing qty)
}

CheckoutResult Store::placeOrder(Customer& customer, Cart& cart, Payment& payment,
                                 const std::string& address) {
    if (cart.isEmpty()) throw InvalidInputException("Your cart is empty.");
    if (customer.isGuest()) throw AuthException("Please log in or register to place an order.");
    if (util::trim(address).size() < 5) {
        throw InvalidInputException("Please provide a delivery address.");
    }

    // Re-check every line against the live catalog (stock may have changed).
    for (const CartItem& item : cart.items()) {
        ProductPtr live = products_.findById(item.productId());
        if (!live) {
            throw NotFoundException("'" + item.product()->getName() +
                                    "' is no longer sold. Please remove it from your cart.");
        }
        if (item.quantity() > live->getStock()) {
            throw OutOfStockException(live->getName(), item.quantity(), live->getStock());
        }
    }

    PriceBreakdown totals = cart.breakdown();
    if (payment.amount() != totals.total) {
        throw PaymentException("Payment amount does not match the order total.");
    }

    std::string reference = payment.pay();   // OOP: runtime polymorphism; may throw

    for (const CartItem& item : cart.items()) {
        item.product()->reduceStock(item.quantity());
    }
    OrderPtr order = Order::fromCart(cart, customer, util::trim(address), util::nowStamp());
    order->setPayment(payment.methodName(), reference);
    orders_.add(order);
    Order::notePlaced();
    cart.clear();

    CheckoutResult result;
    result.order = order;
    try {
        result.invoicePath = invoices_.write(*order, &customer);
    } catch (const FileException& e) {
        result.invoiceError = e.what();
    }
    autosave();
    return result;
}

std::vector<OrderPtr> Store::ordersOf(int customerId) const {
    std::vector<OrderPtr> mine =
        orders_.findAll([customerId](const Order& o) { return o.getCustomerId() == customerId; });
    sortBy(mine, [](const OrderPtr& o) { return o->getId(); }, false);   // OOP: templates
    return mine;
}

std::vector<OrderPtr> Store::allOrdersNewestFirst() const {
    return orders_.sortedBy([](const Order& o) { return o.getId(); }, false);
}

Money Store::totalSpent(int customerId) const {
    std::vector<OrderPtr> mine = ordersOf(customerId);
    return std::accumulate(mine.begin(), mine.end(), Money(), [](const Money& sum, const OrderPtr& o) {
        return o->getStatus() == OrderStatus::Cancelled ? sum : sum + o->total();
    });
}

Money Store::cancelOrder(int orderId, int requesterCustomerId, bool requesterIsAdmin) {
    OrderPtr order = orders_.findById(orderId);
    if (!order || (!requesterIsAdmin && order->getCustomerId() != requesterCustomerId)) {
        throw NotFoundException("Order #" + util::toStr(orderId) + " was not found in your orders.");
    }
    order->cancel();   // throws if already delivered/cancelled

    // Put the items back on the shelf (if the product is still sold).
    for (const OrderLine& l : order->getLines()) {
        ProductPtr p = products_.findById(l.productId);
        if (p) p->restock(l.quantity);
    }

    Money refund;
    bool prepaid = order->getPaymentMethod() != "Cash on Delivery";
    std::shared_ptr<Customer> owner = customers_.findById(order->getCustomerId());
    if (prepaid && owner) {
        refund = order->total();
        owner->creditWallet(refund);
    }
    autosave();
    return refund;
}

void Store::advanceOrder(int orderId) {
    OrderPtr order = orders_.getById(orderId);
    switch (order->getStatus()) {
        case OrderStatus::Placed: order->setStatus(OrderStatus::Shipped); break;
        case OrderStatus::Shipped: order->setStatus(OrderStatus::Delivered); break;
        default:
            throw InvalidInputException("Order #" + util::toStr(orderId) + " is already " +
                                        util::toLower(statusName(order->getStatus())) + ".");
    }
    autosave();
}

ProductPtr Store::addProduct(const std::string& type, const std::string& name, double price,
                             int stock, const std::vector<std::string>& attributes,
                             double rating) {
    ProductPtr p(createProduct(type, Product::generateId(), util::trim(name), price, stock,
                               attributes));
    p->setRating(rating);
    products_.add(p);
    autosave();
    return p;
}

void Store::restock(int productId, int qty) {
    product(productId)->restock(qty);
    autosave();
}

void Store::removeProduct(int productId) {
    if (!products_.remove(productId)) {
        throw NotFoundException("No product with ID " + util::toStr(productId) + ".");
    }
    autosave();
}

void Store::updatePrice(int productId, double price) {
    product(productId)->setPrice(price);
    autosave();
}

std::vector<ProductPtr> Store::lowStock(int threshold) const {
    std::vector<ProductPtr> low =
        products_.findAll([threshold](const Product& p) { return p.isLowStock(threshold); });
    sortBy(low, [](const ProductPtr& p) { return p->getStock(); });
    return low;
}

SalesReport Store::salesReport() const {
    SalesReport r;
    std::map<std::string, int> unitsByProduct;
    std::map<std::string, Money> revenueByProduct;
    for (const OrderPtr& o : orders_) {
        if (o->getStatus() == OrderStatus::Cancelled) {
            ++r.cancelled;
            continue;
        }
        PriceBreakdown t = o->totals();
        r.revenue += t.total;
        r.couponSavings += t.couponDiscount;
        r.orders++;
        r.ordersByPayment[o->getPaymentMethod()]++;
        for (const OrderLine& l : o->getLines()) {
            r.units += l.quantity;
            r.revenueByCategory[l.category] += l.lineTotal();
            r.unitsByCategory[l.category] += l.quantity;
            unitsByProduct[l.name] += l.quantity;
            revenueByProduct[l.name] += l.lineTotal();
        }
    }
    if (r.orders > 0) r.averageOrder = Money::fromPaise(r.revenue.paise() / r.orders);

    r.topProducts.assign(unitsByProduct.begin(), unitsByProduct.end());
    std::stable_sort(r.topProducts.begin(), r.topProducts.end(),
                     [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                         return a.second > b.second;
                     });
    if (r.topProducts.size() > 5) r.topProducts.resize(5);

    r.topRevenue.assign(revenueByProduct.begin(), revenueByProduct.end());
    std::stable_sort(r.topRevenue.begin(), r.topRevenue.end(),
                     [](const std::pair<std::string, Money>& a,
                        const std::pair<std::string, Money>& b) { return a.second > b.second; });
    if (r.topRevenue.size() > 5) r.topRevenue.resize(5);
    return r;
}

} // namespace shop
