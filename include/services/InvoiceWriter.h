#pragma once
#include <string>

#include "cart/Order.h"
#include "core/Customer.h"

namespace shop {

// OOP: file handling -- renders an order as a plain-text tax invoice and
// writes it to <invoiceDir>/INV-<orderId>.txt.
class InvoiceWriter {
public:
    explicit InvoiceWriter(const std::string& directory);

    std::string render(const Order& order, const Customer* customer) const;
    // Returns the file path; throws FileException if the file cannot be written.
    std::string write(const Order& order, const Customer* customer) const;
    std::string pathFor(int orderId) const;
    const std::string& directory() const { return dir_; }

private:
    std::string dir_;
};

} // namespace shop
