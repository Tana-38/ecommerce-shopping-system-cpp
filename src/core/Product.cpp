#include "core/Product.h"
#include <iostream>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

// OOP: static members -- defined once, shared by every Product object.
int Product::nextId_ = 1;
int Product::liveCount_ = 0;

// Encapsulation: all data members live in the base class as protected,
// reachable only through the constructor/getters defined here and in derived classes.
Product::Product(int id, const std::string& name, double price, int stock)
    : id(id), name(name), price(price), stock(stock), rating(4.0) {
    observeId(id);
    ++liveCount_;
}

// OOP: copy constructor -- member-wise copy plus bookkeeping.
Product::Product(const Product& other)
    : id(other.id), name(other.name), price(other.price), stock(other.stock),
      rating(other.rating) {
    ++liveCount_;
}

// OOP: destructor -- runs after the derived destructor when deleting via Product*.
Product::~Product() { --liveCount_; }

// Base implementation of display(); derived classes override this to add
// their own category-specific line while reusing the common fields.
void Product::display() const {
    std::cout << "[" << getCategory() << "] #" << id << " " << name
               << " | Price: Rs " << price
               << " | Discount: Rs " << calculateDiscount()
               << " | Final: Rs " << getFinalPrice()
               << " | Stock: " << stock << "\n";
}

double Product::discountPercent() const {
    return price > 0 ? calculateDiscount() * 100.0 / price : 0.0;
}

int Product::getId() const { return id; }
std::string Product::getName() const { return name; }
double Product::getPrice() const { return price; }
int Product::getStock() const { return stock; }

bool Product::reduceStock(int qty) {
    if (qty < 0 || qty > stock) return false;
    stock -= qty;
    return true;
}

void Product::setName(const std::string& newName) {
    std::string n = util::trim(newName);
    if (n.size() < 2) throw InvalidInputException("Product name must have at least 2 characters.");
    name = n;
}

void Product::setPrice(double newPrice) {
    if (!(newPrice > 0.0) || newPrice > 10000000.0) {
        throw InvalidInputException("Price must be between Rs 0.01 and Rs 1,00,00,000.");
    }
    price = newPrice;
}

void Product::setRating(double newRating) {
    if (newRating < 0.0 || newRating > 5.0) {
        throw InvalidInputException("Rating must be between 0 and 5.");
    }
    rating = newRating;
}

void Product::restock(int qty) {
    if (qty <= 0) throw InvalidInputException("Restock quantity must be positive.");
    if (stock + qty > 100000) throw InvalidInputException("Stock cannot exceed 100000 units.");
    stock += qty;
}

int Product::generateId() { return nextId_++; }

void Product::observeId(int usedId) {
    if (usedId >= nextId_) nextId_ = usedId + 1;
}

int Product::liveCount() { return liveCount_; }

// OOP: friend function + runtime polymorphism -- getCategory()/getFinalPrice()
// dispatch to the real subclass even though we only hold a Product&.
std::ostream& operator<<(std::ostream& os, const Product& p) {
    return os << "#" << p.id << " " << p.name << " [" << p.getType() << "] Rs "
              << util::fixed(p.getFinalPrice(), 2) << " (MRP Rs " << util::fixed(p.price, 2)
              << ", stock " << p.stock << ")";
}

} // namespace shop
