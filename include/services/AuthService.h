#pragma once
#include <memory>
#include <string>

#include "cart/Repository.h"
#include "core/Admin.h"
#include "core/Customer.h"

namespace shop {

// Login / registration rules for customers and the single store admin.
class AuthService {
public:
    explicit AuthService(Repository<Customer>& customers);

    // Throws AuthException on unknown email or wrong password.
    std::shared_ptr<Customer> login(const std::string& email, const std::string& password) const;

    // Validates every field (throws InvalidInputException) and stores the new
    // customer with a welcome bonus in the wallet.
    std::shared_ptr<Customer> registerCustomer(const std::string& name, const std::string& email,
                                               const std::string& phone,
                                               const std::string& password,
                                               const std::string& address);

    // Throws AuthException unless the credentials match (admin / admin123).
    const Admin& adminLogin(const std::string& username, const std::string& password) const;
    const Admin& admin() const { return admin_; }

    // A throw-away customer object for browsing without an account.
    static std::shared_ptr<Customer> guest();

    static void validateName(const std::string& name);
    static void validatePassword(const std::string& password);
    static Money welcomeBonus() { return Money(500.0); }

private:
    Repository<Customer>& customers_;
    Admin admin_;
};

} // namespace shop
