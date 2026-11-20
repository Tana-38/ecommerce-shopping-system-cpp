#include "core/Admin.h"
#include <iostream>

namespace shop {

Admin::Admin(int id, const std::string& name, const std::string& username,
             const std::string& passwordHash)
    : Person(id, name, username + "@shopsphere.in", "", passwordHash), username(username) {}

void Admin::display() const {
    std::cout << "Admin #" << id << " " << name << " (" << username << ")\n";
}

std::string Admin::greeting() const { return "Admin console - signed in as " + username; }

} // namespace shop
