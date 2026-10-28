#pragma once
#include <vector>
#include "Product.h"

// Returns a catalog of 9 heap-allocated products (3 per category), owned by
// the caller. Each slot is a Product* (base pointer) even though the actual
// object is an Electronics/Clothing/Grocery — this is what lets Part B's
// Cart/Order work with any product category uniformly.
std::vector<Product*> createSampleCatalog();

// Deletes every product in the catalog. Because ~Product() is virtual,
// `delete p` here correctly runs the derived class's destructor too.
void freeCatalog(std::vector<Product*>& catalog);
