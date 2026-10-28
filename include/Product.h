#pragma once
#include <string>

// Abstraction: Product defines WHAT every sellable item must support,
// without saying HOW discount/category work for any specific kind of product.
class Product {
protected:
    int id; std::string name; double price; int stock;
public:
    Product(int id, const std::string& name, double price, int stock);
    virtual ~Product() = default;                  // virtual destructor: safe to delete through a Product*
    virtual double calculateDiscount() const = 0;   // discount per unit, in Rs
    virtual std::string getCategory() const = 0;
    virtual void display() const;
    double getFinalPrice() const { return price - calculateDiscount(); }
    int getId() const;
    std::string getName() const;
    double getPrice() const;
    int getStock() const;
    bool reduceStock(int qty);                      // false if not enough stock
};
