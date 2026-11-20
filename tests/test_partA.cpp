#include <iostream>
#include "Catalog.h"
#include "Customer.h"

static void printDivider(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

int main() {
    printDivider("Catalog (runtime polymorphism via Product*)");
    std::vector<Product*> catalog = createSampleCatalog();

    // Each element is a Product* but actually points to an Electronics,
    // Clothing, or Grocery object. calculateDiscount()/getCategory()/display()
    // resolve to the correct override at RUNTIME based on the real object type
    // (dynamic dispatch through the vtable) -- that's polymorphism in action.
    double totalFinalValue = 0.0;
    for (const Product* p : catalog) {
        p->display();
        totalFinalValue += p->getFinalPrice();
    }
    std::cout << "\nSum of final prices across catalog: Rs " << totalFinalValue << "\n";

    printDivider("reduceStock edge cases");
    Product* laptop = catalog[2]; // Laptop, stock = 5
    std::cout << "Laptop stock before: " << laptop->getStock() << "\n";
    std::cout << "reduceStock(3) -> " << std::boolalpha << laptop->reduceStock(3)
               << " (stock now " << laptop->getStock() << ")\n";
    std::cout << "reduceStock(10) -> " << laptop->reduceStock(10)
               << " (not enough stock, stock stays " << laptop->getStock() << ")\n";
    std::cout << "reduceStock(-1) -> " << laptop->reduceStock(-1)
               << " (negative qty rejected, stock stays " << laptop->getStock() << ")\n";

    printDivider("Customer creation and validation");
    Customer cust(101, "Riya Sharma", "riya.sharma@example.com", "9876543210", "Pune, Maharashtra");
    cust.display();

    std::cout << "\nsetEmail(\"not-an-email\") -> " << cust.setEmail("not-an-email")
               << " (rejected, still " << cust.getEmail() << ")\n";
    std::cout << "setEmail(\"riya@new.com\") -> " << cust.setEmail("riya@new.com")
               << " (accepted, now " << cust.getEmail() << ")\n";

    std::cout << "setPhone(\"12345\") -> " << cust.setPhone("12345")
               << " (rejected, not 10 digits, still " << cust.getPhone() << ")\n";
    std::cout << "setPhone(\"98765abcde\") -> " << cust.setPhone("98765abcde")
               << " (rejected, not all digits, still " << cust.getPhone() << ")\n";
    std::cout << "setPhone(\"9123456780\") -> " << cust.setPhone("9123456780")
               << " (accepted, now " << cust.getPhone() << ")\n";

    freeCatalog(catalog); // virtual ~Product() -> every derived object's destructor runs correctly
    std::cout << "\nCatalog freed. Done.\n";
    return 0;
}
