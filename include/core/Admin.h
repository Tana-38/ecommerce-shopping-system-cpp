#pragma once
#include <string>

#include "core/Person.h"

namespace shop {

// OOP: inheritance (hierarchical) -- Admin and Customer are siblings under Person.
class Admin : public Person {
private:
    std::string username;

public:
    Admin(int id, const std::string& name, const std::string& username,
          const std::string& passwordHash);

    const std::string& getUsername() const { return username; }

    // OOP: runtime polymorphism -- role-specific overrides.
    std::string getRole() const override { return "Administrator"; }
    void display() const override;
    std::string greeting() const override;
};

} // namespace shop
