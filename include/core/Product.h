#pragma once
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

// OOP: namespaces -- every class in the project lives in namespace shop
// (console UI in shop::ui, helpers in shop::util), so names never clash.
namespace shop {

// Abstraction: Product defines WHAT every sellable item must support,
// without saying HOW discount/category work for any specific kind of product.
// OOP: abstraction -- abstract base class with pure virtual functions.
class Product {
protected:
    // OOP: encapsulation -- state is hidden; derived classes may read it,
    // outside code must go through getters and validated setters.
    int id; std::string name; double price; int stock;
    double rating;

public:
    // OOP: constructors -- parameterised constructor (Part A contract).
    Product(int id, const std::string& name, double price, int stock);
    // OOP: copy constructor -- used by clone(); keeps the live-object counter right.
    Product(const Product& other);
    Product& operator=(const Product&) = delete;   // no slicing assignments

    // OOP: destructor -- virtual, so deleting through a Product* runs the
    // derived destructor first (safe polymorphic deletion).
    virtual ~Product();

    // OOP: runtime polymorphism -- pure virtual hooks every category implements.
    virtual double calculateDiscount() const = 0;   // discount per unit, in Rs
    virtual std::string getCategory() const = 0;
    virtual std::string discountRule() const = 0;   // human-readable offer text
    virtual std::string attributeSummary() const = 0;           // e.g. "12-month warranty"
    virtual std::vector<std::string> attributeFields() const = 0;  // for CSV
    virtual std::unique_ptr<Product> clone() const = 0;
    // Concrete type name; differs from the category for deeper subclasses
    // (a Smartphone's category is still "Electronics").
    virtual std::string getType() const { return getCategory(); }
    virtual void display() const;

    double getFinalPrice() const { return price - calculateDiscount(); }
    double discountPercent() const;
    int getId() const;
    std::string getName() const;
    double getPrice() const;
    int getStock() const;
    double getRating() const { return rating; }
    bool isLowStock(int threshold = 5) const { return stock <= threshold; }

    bool reduceStock(int qty);                      // false if not enough stock

    // OOP: encapsulation -- validated setters throw InvalidInputException.
    void setName(const std::string& newName);
    void setPrice(double newPrice);
    void setRating(double newRating);
    void restock(int qty);

    // OOP: static members -- ID generator and live-object counter shared by all products.
    static int generateId();
    static void observeId(int usedId);
    static int liveCount();

    // OOP: friend function -- stream insertion for any product via a base reference.
    friend std::ostream& operator<<(std::ostream& os, const Product& p);

private:
    static int nextId_;
    static int liveCount_;
};

using ProductPtr = std::shared_ptr<Product>;

} // namespace shop
