# E-Commerce Shopping System (C++ OOP Mini Project)

A terminal-based (no GUI) C++ project built for a college OOP course. It models a simple
e-commerce flow: products (with category-specific discount rules), customers, a shopping
cart, orders, and payment — built around the four pillars of OOP (encapsulation,
abstraction, inheritance, polymorphism).

## Team split

| Part | Scope | Owner |
|------|-------|-------|
| Part A | `Product` hierarchy (Electronics/Clothing/Grocery), `Customer`, `Catalog`, Part A demo/tests | Tanaya (this branch) |
| Part B | `Cart`, `Order`, `Payment`, `main.cpp` menu | Teammate (separate branch) |

Part A and Part B share one contract: **`include/Product.h`**. Part B consumes products
only through `Product*` (base class pointers) and `getFinalPrice()` — it never needs to
know which concrete subclass (Electronics/Clothing/Grocery) it's holding.

## Project structure

```
include/   all header (.h) files
src/       all implementation (.cpp) files
tests/     Part A demo/test driver (test_partA.cpp)
```

## Build & run

### Part A demo

```
g++ -std=c++17 -Wall -Wextra -Iinclude src/*.cpp tests/test_partA.cpp -o test_partA
./test_partA
```

### Full project (once Part B/main.cpp lands)

```
g++ -std=c++17 -Wall -Wextra -Iinclude src/*.cpp -o shopping_system
./shopping_system
```

## Which OOP concept is where

| Concept | Where |
|---|---|
| Abstraction | `Product` is an abstract base class — it declares *what* a product must do (`calculateDiscount`, `getCategory`) without fixing *how* |
| Encapsulation | `protected`/`private` data members in `Product` and `Customer`, accessed only via getters/validated setters |
| Inheritance | `Electronics`, `Clothing`, `Grocery` all derive from `Product` and extend it with their own field |
| Runtime polymorphism | `tests/test_partA.cpp` loops over `std::vector<Product*>` and calls `calculateDiscount()`/`getCategory()`/`display()` — the correct override runs based on the actual object type, decided at runtime via the vtable |
| Virtual destructor | `virtual ~Product() = default;` ensures deleting a `Product*` that actually points to a derived object (e.g. `Electronics`) destructs it correctly, with no undefined behaviour / resource leak |
