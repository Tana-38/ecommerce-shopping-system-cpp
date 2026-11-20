#pragma once
#include <memory>
#include <string>
#include <vector>

#include "cart/Order.h"
#include "core/Customer.h"
#include "core/Product.h"

namespace shop {

// OOP: file handling -- reads and writes the store's CSV files with fstream.
//   products.csv     id,type,name,price,stock,rating,attr1,attr2
//   customers.csv    id,name,email,phone,address,password_hash,wallet,joined
//   orders.csv       id,customer_id,customer_name,date,status,payment_method,payment_ref,coupon,address,total
//   order_items.csv  order_id,product_id,name,category,unit_price,unit_discount,qty
class FileStore {
public:
    explicit FileStore(const std::string& directory);

    const std::string& directory() const { return dir_; }
    bool hasData() const;   // true when products.csv exists

    std::vector<ProductPtr> loadProducts() const;
    std::vector<std::shared_ptr<Customer>> loadCustomers() const;
    std::vector<OrderPtr> loadOrders() const;

    void saveProducts(const std::vector<ProductPtr>& products) const;
    void saveCustomers(const std::vector<std::shared_ptr<Customer>>& customers) const;
    void saveOrders(const std::vector<OrderPtr>& orders) const;

    int skippedRows() const { return skipped_; }

    // CSV helpers (quoted fields, embedded commas and "" escapes supported).
    static std::vector<std::string> parseCsvLine(const std::string& line);
    static std::string toCsvLine(const std::vector<std::string>& fields);

    // Tiny portable filesystem helpers (no <filesystem> on older compilers).
    static bool fileExists(const std::string& path);
    static bool ensureDirectory(const std::string& path);
    static std::string join(const std::string& dir, const std::string& file);

private:
    std::string dir_;
    mutable int skipped_;   // malformed rows ignored during the last load

    std::vector<std::vector<std::string>> readRows(const std::string& file) const;
    void writeRows(const std::string& file, const std::vector<std::string>& header,
                   const std::vector<std::vector<std::string>>& rows) const;
};

} // namespace shop
