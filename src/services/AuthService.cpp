#include "services/AuthService.h"

#include <cctype>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

AuthService::AuthService(Repository<Customer>& customers)
    : customers_(customers), admin_(1, "Store Admin", "admin", util::hashPassword("admin123")) {}

std::shared_ptr<Customer> AuthService::login(const std::string& email,
                                             const std::string& password) const {
    std::string wanted = util::toLower(util::trim(email));
    if (wanted.empty()) throw AuthException("Please enter your email address.");
    std::shared_ptr<Customer> c = customers_.findFirst(
        [&](const Customer& x) { return util::toLower(x.getEmail()) == wanted; });
    if (!c) throw AuthException("No account found for " + wanted + ".");
    if (!c->checkPassword(password)) throw AuthException("Incorrect password. Please try again.");
    return c;
}

void AuthService::validateName(const std::string& name) {
    std::string n = util::trim(name);
    int letters = 0;
    for (char ch : n) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalpha(c)) ++letters;
        else if (c != ' ' && c != '.' && c != '\'' && c != '-') {
            throw InvalidInputException("Name may contain only letters, spaces, '.', '-' and '\''.");
        }
    }
    if (letters < 2) throw InvalidInputException("Name must have at least 2 letters.");
}

void AuthService::validatePassword(const std::string& password) {
    if (password.size() < 6) throw InvalidInputException("Password must be at least 6 characters.");
    bool letter = false;
    bool digit = false;
    for (char ch : password) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalpha(c)) letter = true;
        if (std::isdigit(c)) digit = true;
    }
    if (!letter || !digit) {
        throw InvalidInputException("Password needs at least one letter and one digit.");
    }
}

std::shared_ptr<Customer> AuthService::registerCustomer(const std::string& name,
                                                        const std::string& email,
                                                        const std::string& phone,
                                                        const std::string& password,
                                                        const std::string& address) {
    validateName(name);
    std::string mail = util::toLower(util::trim(email));
    if (!Customer::isValidEmail(mail)) {
        throw InvalidInputException("Please enter a valid email like name@example.com.");
    }
    if (customers_.findFirst([&](const Customer& c) { return util::toLower(c.getEmail()) == mail; })) {
        throw InvalidInputException("An account with " + mail + " already exists.");
    }
    std::string ph = util::trim(phone);
    if (!Customer::isValidPhone(ph)) {
        throw InvalidInputException("Phone number must be exactly 10 digits.");
    }
    validatePassword(password);
    if (util::trim(address).size() < 5) {
        throw InvalidInputException("Please enter a delivery address (at least 5 characters).");
    }
    auto c = std::make_shared<Customer>(Customer::generateId(), util::trim(name), mail, ph,
                                        util::trim(address), util::hashPassword(password),
                                        welcomeBonus(), util::nowStamp().substr(0, 10));
    customers_.add(c);
    return c;
}

const Admin& AuthService::adminLogin(const std::string& username,
                                     const std::string& password) const {
    if (util::trim(username) != admin_.getUsername() || !admin_.checkPassword(password)) {
        throw AuthException("Invalid admin credentials.");
    }
    return admin_;
}

std::shared_ptr<Customer> AuthService::guest() {
    return std::make_shared<Customer>(0, "Guest Shopper", "guest@shopsphere.in", "", "",
                                      "", Money(), util::nowStamp().substr(0, 10));
}

} // namespace shop
