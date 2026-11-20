#include "core/Person.h"

#include "util/StringUtil.h"

namespace shop {

Person::Person() : id(0) {}

Person::Person(int id, const std::string& name, const std::string& email,
               const std::string& phone, const std::string& passwordHash)
    : id(id), name(name), email(email), phone(phone), passwordHash(passwordHash) {}

Person::~Person() {}

std::string Person::greeting() const { return "Welcome, " + firstName(); }

int Person::getId() const { return id; }
std::string Person::getName() const { return name; }
std::string Person::getEmail() const { return email; }
std::string Person::getPhone() const { return phone; }

std::string Person::firstName() const {
    std::size_t space = name.find(' ');
    return space == std::string::npos ? name : name.substr(0, space);
}

bool Person::checkPassword(const std::string& plain) const {
    return !passwordHash.empty() && util::hashPassword(plain) == passwordHash;
}

void Person::setPassword(const std::string& plain) {
    passwordHash = util::hashPassword(plain);
}

} // namespace shop
