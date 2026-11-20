#pragma once
#include <string>
#include <utility>
#include <vector>

#include "cart/Cart.h"
#include "cart/Order.h"
#include "core/Catalog.h"
#include "core/Customer.h"
#include "core/Product.h"
#include "services/Store.h"
#include "util/Exceptions.h"

namespace shop {
namespace ui {

const int kPageWidth = 96;   // every screen is laid out for ~100 terminal columns

enum class Tone { Success, Error, Warning, Info };

typedef std::vector<std::pair<std::string, std::string>> KeyHints;

// Reusable renderers shared by the interactive App and the scripted Demo.
// Functions returning std::string build a frame; void functions print.
namespace screens {

// ---- chrome -----------------------------------------------------------------
std::vector<std::string> logoLines(double phase);
void splash(bool showLoading);
std::string header(const std::string& crumb, const std::string& right);
std::string footer(const KeyHints& keys);
void begin(const std::string& crumb, const std::string& right);   // clear + header
void sectionTitle(const std::string& title, const std::string& subtitle = "");

// ---- feedback & input ---------------------------------------------------------
void notify(Tone tone, const std::string& text);
void showError(const ShopException& e);
void waitKey(const std::string& text = "Press any key to continue");
std::string prompt(const std::string& label, const std::string& hint = "");
std::string promptSecret(const std::string& label);
bool confirm(const std::string& question);
// Demo helper: shows a prompt and "types" the answer.
void fakeInput(const std::string& label, const std::string& value, bool secret = false);

// ---- building blocks ------------------------------------------------------------
std::string categoryTag(const std::string& category);
std::string statusBadge(OrderStatus status);
std::string stockText(int stock);
std::string shortPayment(const std::string& method);
std::string filterBar(const std::string& category, SortKey sort, const std::string& search,
                      std::size_t count);
std::string stepper(int step);   // 0 Address, 1 Payment, 2 Done

// ---- catalog / cart / orders -------------------------------------------------------
std::string productTable(const std::vector<ProductPtr>& items, int selected, int offset,
                         int pageSize);
std::string productCard(const Product& p, int inCart);
std::string cartTable(const Cart& cart, int selected);
std::vector<std::string> breakdownLines(const PriceBreakdown& b, int width);
std::string cartSummary(const Cart& cart);   // breakdown + offers side by side
std::string orderTable(const std::vector<OrderPtr>& orders, int selected, bool showCustomer);
std::string orderDetail(const Order& order);
std::string profileCard(const Customer& c, int orderCount, const Money& spent);

// ---- admin ----------------------------------------------------------------------
std::string kpiTiles(const SalesReport& r, std::size_t productCount, std::size_t customerCount);
std::vector<std::string> salesCharts(const SalesReport& r, double progress);
void animatedSalesCharts(const SalesReport& r);
std::string lowStockPanel(const std::vector<ProductPtr>& low);

}  // namespace screens
}  // namespace ui
}  // namespace shop
