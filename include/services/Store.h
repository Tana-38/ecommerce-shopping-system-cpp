#pragma once
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cart/Cart.h"
#include "cart/Order.h"
#include "cart/Repository.h"
#include "core/Catalog.h"
#include "core/Customer.h"
#include "core/Product.h"
#include "payment/Payment.h"
#include "services/AuthService.h"
#include "services/FileStore.h"
#include "services/InvoiceWriter.h"

namespace shop {

// Aggregated numbers for the admin dashboard.
struct SalesReport {
    Money revenue;                 // grand totals of non-cancelled orders
    Money couponSavings;
    Money averageOrder;
    int orders = 0;                // non-cancelled
    int cancelled = 0;
    int units = 0;
    std::map<std::string, Money> revenueByCategory;   // net item value per category
    std::map<std::string, int> unitsByCategory;
    std::map<std::string, int> ordersByPayment;
    std::vector<std::pair<std::string, int>> topProducts;      // by units sold
    std::vector<std::pair<std::string, Money>> topRevenue;     // by item revenue
};

struct CheckoutResult {
    OrderPtr order;
    std::string invoicePath;    // empty if the invoice could not be written
    std::string invoiceError;
};

// OOP: classes & objects -- Store is the facade the UI talks to. It owns the
// repositories (shared_ptr items), the auth service and the file store
// (unique_ptr), and implements every business rule in one place.
class Store {
public:
    Store(const std::string& dataDir = "data", const std::string& invoiceDir = "invoices");
    Store(const Store&) = delete;              // the store is a unique resource
    Store& operator=(const Store&) = delete;
    ~Store();

    void load();       // data/*.csv, or the built-in seed when the folder is empty
    void loadSeed();   // built-in seed only, nothing read from disk
    void save() const;
    void setPersistence(bool enabled) { persist_ = enabled; }
    bool persistence() const { return persist_; }
    int skippedRows() const { return files_->skippedRows(); }
    const std::string& dataDir() const { return files_->directory(); }

    // ---- catalog ------------------------------------------------------------
    Catalog catalog() const { return Catalog(products_.all()); }
    const Repository<Product>& products() const { return products_; }
    ProductPtr product(int id) const;   // throws NotFoundException

    // ---- customers ----------------------------------------------------------
    AuthService& auth() { return *auth_; }
    const Repository<Customer>& customers() const { return customers_; }
    void saveCustomers() const;   // after profile/wallet edits

    // ---- cart & checkout ----------------------------------------------------
    void addToCart(Cart& cart, int productId, int qty) const;
    CheckoutResult placeOrder(Customer& customer, Cart& cart, Payment& payment,
                              const std::string& address);

    // ---- orders -------------------------------------------------------------
    const Repository<Order>& orders() const { return orders_; }
    std::vector<OrderPtr> ordersOf(int customerId) const;   // newest first
    std::vector<OrderPtr> allOrdersNewestFirst() const;
    Money totalSpent(int customerId) const;
    // Cancels (restocks items, refunds prepaid amount to the wallet).
    // Returns the refunded amount. Customers may cancel only their own orders.
    Money cancelOrder(int orderId, int requesterCustomerId, bool requesterIsAdmin);
    void advanceOrder(int orderId);   // admin: Placed -> Shipped -> Delivered
    const InvoiceWriter& invoices() const { return invoices_; }

    // ---- admin --------------------------------------------------------------
    ProductPtr addProduct(const std::string& type, const std::string& name, double price,
                          int stock, const std::vector<std::string>& attributes, double rating);
    void restock(int productId, int qty);
    void removeProduct(int productId);
    void updatePrice(int productId, double price);
    std::vector<ProductPtr> lowStock(int threshold = 5) const;
    SalesReport salesReport() const;

private:
    Repository<Product> products_;
    Repository<Customer> customers_;
    Repository<Order> orders_;
    // OOP: smart pointers -- exclusive ownership of helper services.
    std::unique_ptr<FileStore> files_;
    std::unique_ptr<AuthService> auth_;
    InvoiceWriter invoices_;
    bool persist_;

    void autosave() const;
};

} // namespace shop
