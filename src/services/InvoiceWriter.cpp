#include "services/InvoiceWriter.h"

#include <fstream>
#include <sstream>

#include "services/FileStore.h"
#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

namespace {
const int kWidth = 66;

std::string rule(char c) { return std::string(kWidth, c); }

std::string amountRow(const std::string& label, const std::string& amount) {
    return util::padLeft(label, 46) + util::padLeft(amount, kWidth - 46);
}
}  // namespace

InvoiceWriter::InvoiceWriter(const std::string& directory) : dir_(directory) {}

std::string InvoiceWriter::pathFor(int orderId) const {
    return FileStore::join(dir_, "INV-" + util::toStr(orderId) + ".txt");
}

std::string InvoiceWriter::render(const Order& order, const Customer* customer) const {
    using util::padLeft;
    using util::padRight;
    std::ostringstream out;
    PriceBreakdown t = order.totals();

    out << rule('=') << "\n";
    out << util::center("S H O P S P H E R E", kWidth) << "\n";
    out << util::center("Tax Invoice  |  shop smarter, live better", kWidth) << "\n";
    out << rule('=') << "\n";
    out << padRight("Invoice No : INV-" + util::toStr(order.getId()), 34)
        << "Date    : " << order.getDate() << "\n";
    out << padRight("Order No   : #" + util::toStr(order.getId()), 34)
        << "Payment : " << order.getPaymentMethod() << "\n";
    out << padRight("Txn Ref    : " + order.getPaymentRef(), 34)
        << "Status  : " << statusName(order.getStatus()) << "\n";
    out << rule('-') << "\n";
    out << "Bill to : " << order.getCustomerName();
    if (customer) {
        out << "  |  " << customer->getEmail();
        if (!customer->getPhone().empty()) out << "  |  " << customer->getPhone();
    }
    out << "\n";
    out << "Ship to : " << order.getAddress() << "\n";
    out << rule('-') << "\n";
    out << padRight("#", 4) << padRight("Item", 30) << padLeft("Qty", 4) << padLeft("Unit", 13)
        << padLeft("Amount", 15) << "\n";
    out << rule('-') << "\n";
    int n = 1;
    for (const OrderLine& l : order.getLines()) {
        out << padRight(util::toStr(n++), 4) << padRight(util::truncate(l.name, 29), 30)
            << padLeft(util::toStr(l.quantity), 4) << padLeft(l.unitFinal().plain(), 13)
            << padLeft(l.lineTotal().plain(), 15) << "\n";
        if (!l.unitDiscount.isZero()) {
            out << "    " << l.category << " offer: MRP " << l.unitPrice.plain() << ", you save "
                << l.unitDiscount.plain() << " each\n";
        }
    }
    out << rule('-') << "\n";
    out << amountRow("MRP total", t.mrpTotal.plain()) << "\n";
    out << amountRow("Product discounts", "-" + t.productDiscount.plain()) << "\n";
    out << amountRow("Subtotal", t.subtotal.plain()) << "\n";
    if (!t.couponDiscount.isZero()) {
        out << amountRow("Coupon " + t.couponCode, "-" + t.couponDiscount.plain()) << "\n";
    }
    out << amountRow("Delivery", t.deliveryFee.isZero() ? "FREE" : t.deliveryFee.plain()) << "\n";
    out << amountRow("GST @ 5%", t.gst.plain()) << "\n";
    out << rule('-') << "\n";
    out << amountRow("GRAND TOTAL", t.total.plain()) << "\n";
    out << rule('=') << "\n";
    out << util::center("You saved " + t.totalSavings().plain() + " on this order!", kWidth)
        << "\n";
    out << util::center("Thank you for shopping with ShopSphere.", kWidth) << "\n";
    out << util::center("This is a computer generated invoice.", kWidth) << "\n";
    out << rule('=') << "\n";
    return out.str();
}

std::string InvoiceWriter::write(const Order& order, const Customer* customer) const {
    if (!FileStore::ensureDirectory(dir_)) {
        throw FileException("Cannot create invoice folder '" + dir_ + "'.");
    }
    std::string path = pathFor(order.getId());
    std::ofstream out(path.c_str(), std::ios::trunc);   // OOP: file handling -- write invoice
    if (!out) throw FileException("Cannot write invoice '" + path + "'.");
    out << render(order, customer);
    return path;
}

} // namespace shop
