#pragma once
#include <memory>
#include <vector>

#include "cart/Order.h"
#include "core/Customer.h"
#include "core/Product.h"

namespace shop {
namespace seed {

// Built-in starter data. Used to create data/*.csv on first run and as the
// fixed, reproducible dataset for --demo mode.
std::vector<ProductPtr> products();                     // 23 products
std::vector<std::shared_ptr<Customer>> customers();     // 4 demo customers (password pass123)
std::vector<OrderPtr> orders(const std::vector<ProductPtr>& products,
                             const std::vector<std::shared_ptr<Customer>>& customers);

}  // namespace seed
}  // namespace shop
