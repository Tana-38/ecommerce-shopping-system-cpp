#include "services/FileStore.h"

#include <fstream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "core/Catalog.h"
#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

FileStore::FileStore(const std::string& directory) : dir_(directory), skipped_(0) {}

bool FileStore::hasData() const { return fileExists(join(dir_, "products.csv")); }

std::string FileStore::join(const std::string& dir, const std::string& file) {
    if (dir.empty()) return file;
    char last = dir[dir.size() - 1];
    return (last == '/' || last == '\\') ? dir + file : dir + "/" + file;
}

bool FileStore::fileExists(const std::string& path) {
    std::ifstream in(path.c_str());
    return in.good();
}

bool FileStore::ensureDirectory(const std::string& path) {
    if (path.empty() || path == ".") return true;
    struct stat info;
    if (stat(path.c_str(), &info) == 0) return (info.st_mode & S_IFDIR) != 0;
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

std::vector<std::string> FileStore::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                cur += c;
            }
        } else if (c == '"') {
            quoted = true;
        } else if (c == ',') {
            fields.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur += c;
        }
    }
    fields.push_back(cur);
    return fields;
}

std::string FileStore::toCsvLine(const std::vector<std::string>& fields) {
    std::string out;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i) out += ',';
        const std::string& f = fields[i];
        if (f.find_first_of(",\"\n") != std::string::npos) {
            out += '"' + util::replaceAll(f, "\"", "\"\"") + '"';
        } else {
            out += f;
        }
    }
    return out;
}

std::vector<std::vector<std::string>> FileStore::readRows(const std::string& file) const {
    std::vector<std::vector<std::string>> rows;
    std::ifstream in(join(dir_, file).c_str());   // OOP: file handling -- ifstream
    if (!in) return rows;
    std::string line;
    bool header = true;
    while (std::getline(in, line)) {
        if (header) {   // first line is the column header
            header = false;
            continue;
        }
        if (util::trim(line).empty()) continue;
        rows.push_back(parseCsvLine(line));
    }
    return rows;
}

void FileStore::writeRows(const std::string& file, const std::vector<std::string>& header,
                          const std::vector<std::vector<std::string>>& rows) const {
    if (!ensureDirectory(dir_)) throw FileException("Cannot create folder '" + dir_ + "'.");
    std::string path = join(dir_, file);
    std::ofstream out(path.c_str(), std::ios::trunc);   // OOP: file handling -- ofstream
    if (!out) throw FileException("Cannot write '" + path + "'.");
    out << toCsvLine(header) << "\n";
    for (const auto& r : rows) out << toCsvLine(r) << "\n";
    if (!out) throw FileException("Write to '" + path + "' failed.");
}

std::vector<ProductPtr> FileStore::loadProducts() const {
    skipped_ = 0;
    std::vector<ProductPtr> products;
    for (const auto& r : readRows("products.csv")) {
        try {
            if (r.size() < 7) throw InvalidInputException("short row");
            std::vector<std::string> attrs(r.begin() + 6, r.end());
            ProductPtr p(createProduct(r[1], util::toIntOr(r[0], -1), r[2],
                                       util::toDoubleOr(r[3], -1.0), util::toIntOr(r[4], -1),
                                       attrs));
            if (p->getId() <= 0) throw InvalidInputException("bad id");
            p->setRating(util::toDoubleOr(r[5], 4.0));
            products.push_back(p);
        } catch (const ShopException&) {
            ++skipped_;   // OOP: exception handling -- skip malformed rows, keep loading
        }
    }
    return products;
}

std::vector<std::shared_ptr<Customer>> FileStore::loadCustomers() const {
    std::vector<std::shared_ptr<Customer>> customers;
    for (const auto& r : readRows("customers.csv")) {
        try {
            if (r.size() < 8) throw InvalidInputException("short row");
            int id = util::toIntOr(r[0], -1);
            if (id <= 0) throw InvalidInputException("bad id");
            customers.push_back(std::make_shared<Customer>(id, r[1], r[2], r[3], r[4], r[5],
                                                           Money::parse(r[6]), r[7]));
        } catch (const ShopException&) {
            ++skipped_;
        }
    }
    return customers;
}

std::vector<OrderPtr> FileStore::loadOrders() const {
    // Group item rows by order id first.
    std::map<int, std::vector<OrderLine>> linesByOrder;   // OOP: STL containers -- std::map
    for (const auto& r : readRows("order_items.csv")) {
        try {
            if (r.size() < 7) throw InvalidInputException("short row");
            OrderLine l;
            l.productId = util::toIntOr(r[1], 0);
            l.name = r[2];
            l.category = r[3];
            l.unitPrice = Money::parse(r[4]);
            l.unitDiscount = Money::parse(r[5]);
            l.quantity = util::toIntOr(r[6], 0);
            if (l.quantity <= 0) throw InvalidInputException("bad qty");
            linesByOrder[util::toIntOr(r[0], 0)].push_back(l);
        } catch (const ShopException&) {
            ++skipped_;
        }
    }
    std::vector<OrderPtr> orders;
    for (const auto& r : readRows("orders.csv")) {
        if (r.size() < 9) {
            ++skipped_;
            continue;
        }
        int id = util::toIntOr(r[0], 0);
        auto it = linesByOrder.find(id);
        if (id <= 0 || it == linesByOrder.end()) {
            ++skipped_;
            continue;
        }
        OrderPtr o = std::make_shared<Order>(id, util::toIntOr(r[1], 0), r[2], r[3], r[8],
                                             it->second, r[7]);
        o->setStatus(statusFromName(r[4]));
        o->setPayment(r[5], r[6]);
        orders.push_back(o);
    }
    return orders;
}

void FileStore::saveProducts(const std::vector<ProductPtr>& products) const {
    std::vector<std::vector<std::string>> rows;
    for (const ProductPtr& p : products) {
        std::vector<std::string> row = {util::toStr(p->getId()), p->getType(), p->getName(),
                                        util::fixed(p->getPrice(), 2), util::toStr(p->getStock()),
                                        util::fixed(p->getRating(), 1)};
        // OOP: runtime polymorphism -- each subclass serialises its own fields.
        std::vector<std::string> attrs = p->attributeFields();
        attrs.resize(2);
        row.insert(row.end(), attrs.begin(), attrs.end());
        rows.push_back(row);
    }
    writeRows("products.csv", {"id", "type", "name", "price", "stock", "rating", "attr1", "attr2"},
              rows);
}

void FileStore::saveCustomers(const std::vector<std::shared_ptr<Customer>>& customers) const {
    std::vector<std::vector<std::string>> rows;
    for (const auto& c : customers) {
        rows.push_back({util::toStr(c->getId()), c->getName(), c->getEmail(), c->getPhone(),
                        c->getAddress(), c->getPasswordHash(), c->getWallet().number(),
                        c->getJoinedOn()});
    }
    writeRows("customers.csv",
              {"id", "name", "email", "phone", "address", "password_hash", "wallet", "joined"},
              rows);
}

void FileStore::saveOrders(const std::vector<OrderPtr>& orders) const {
    std::vector<std::vector<std::string>> orderRows;
    std::vector<std::vector<std::string>> itemRows;
    for (const OrderPtr& o : orders) {
        orderRows.push_back({util::toStr(o->getId()), util::toStr(o->getCustomerId()),
                             o->getCustomerName(), o->getDate(), statusName(o->getStatus()),
                             o->getPaymentMethod(), o->getPaymentRef(), o->getCouponCode(),
                             o->getAddress(), o->total().number()});
        for (const OrderLine& l : o->getLines()) {
            itemRows.push_back({util::toStr(o->getId()), util::toStr(l.productId), l.name,
                                l.category, l.unitPrice.number(), l.unitDiscount.number(),
                                util::toStr(l.quantity)});
        }
    }
    writeRows("orders.csv",
              {"id", "customer_id", "customer_name", "date", "status", "payment_method",
               "payment_ref", "coupon", "address", "total"},
              orderRows);
    writeRows("order_items.csv",
              {"order_id", "product_id", "name", "category", "unit_price", "unit_discount", "qty"},
              itemRows);
}

} // namespace shop
