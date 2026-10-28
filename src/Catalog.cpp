#include "Catalog.h"
#include "Electronics.h"
#include "Clothing.h"
#include "Grocery.h"

std::vector<Product*> createSampleCatalog() {
    std::vector<Product*> catalog;

    // Electronics: id, name, price (Rs), stock, warrantyMonths
    catalog.push_back(new Electronics(1, "Bluetooth Earbuds", 1999.0, 25, 6));
    catalog.push_back(new Electronics(2, "Smartphone", 18999.0, 10, 12));
    catalog.push_back(new Electronics(3, "Laptop", 52999.0, 5, 24));

    // Clothing: id, name, price (Rs), stock, size
    catalog.push_back(new Clothing(4, "Cotton T-Shirt", 499.0, 50, "M"));
    catalog.push_back(new Clothing(5, "Denim Jacket", 1899.0, 20, "L"));
    catalog.push_back(new Clothing(6, "Formal Shirt", 1199.0, 30, "XL"));

    // Grocery: id, name, price (Rs), stock, expiryDate
    catalog.push_back(new Grocery(7, "Basmati Rice 5kg", 450.0, 100, "2026-06-30"));
    catalog.push_back(new Grocery(8, "Sunflower Oil 1L", 180.0, 80, "2026-03-15"));
    catalog.push_back(new Grocery(9, "Toor Dal 1kg", 15.0, 60, "2026-04-20")); // price < discount

    return catalog;
}

void freeCatalog(std::vector<Product*>& catalog) {
    for (Product* p : catalog) {
        delete p;   // virtual ~Product() ensures the correct derived destructor runs
    }
    catalog.clear();
}
