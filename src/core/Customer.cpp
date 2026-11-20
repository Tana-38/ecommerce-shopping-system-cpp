#include "core/Customer.h"
#include <iostream>

#include "util/Exceptions.h"
#include "util/StringUtil.h"

namespace shop {

int Customer::nextId_ = 1;

Customer::Customer(int id, const std::string& name, const std::string& email,
                     const std::string& phone, const std::string& address)
    : Person(id, name, email, phone, ""), address(address), wallet(),
      joinedOn(util::nowStamp().substr(0, 10)) {
    observeId(id);
}

Customer::Customer(int id, const std::string& name, const std::string& email,
                   const std::string& phone, const std::string& address,
                   const std::string& passwordHash, const Money& wallet,
                   const std::string& joinedOn)
    : Person(id, name, email, phone, passwordHash), address(address), wallet(wallet),
      joinedOn(joinedOn) {
    observeId(id);
}

std::string Customer::getAddress() const { return address; }

void Customer::setName(const std::string& newName) { name = newName; }

bool Customer::setEmail(const std::string& newEmail) {
    if (!isValidEmail(newEmail)) return false;
    email = newEmail;
    return true;
}

bool Customer::setPhone(const std::string& newPhone) {
    if (!isValidPhone(newPhone)) return false;
    phone = newPhone;
    return true;
}

void Customer::setAddress(const std::string& newAddress) { address = newAddress; }

void Customer::creditWallet(const Money& amount) {
    if (amount <= Money()) throw InvalidInputException("Amount must be positive.");
    wallet += amount;
}

void Customer::debitWallet(const Money& amount) {
    if (amount <= Money()) throw InvalidInputException("Amount must be positive.");
    if (amount > wallet) {
        throw PaymentException("Insufficient wallet balance (" + wallet.plain() + ").");
    }
    wallet -= amount;
}

void Customer::display() const {
    std::cout << "Customer #" << id << " " << name
               << " | Email: " << email
               << " | Phone: " << phone
               << " | Address: " << address << "\n";
}

bool Customer::isValidEmail(const std::string& email) {
    return util::isValidEmail(email);
}

bool Customer::isValidPhone(const std::string& phone) {
    return util::isValidPhone(phone);
}

int Customer::generateId() { return nextId_++; }

void Customer::observeId(int usedId) {
    if (usedId >= nextId_) nextId_ = usedId + 1;
}

} // namespace shop
