#include "Customer.h"
#include <algorithm>
#include <cctype>
#include <iostream>

Customer::Customer(int id, const std::string& name, const std::string& email,
                     const std::string& phone, const std::string& address)
    : id(id), name(name), email(email), phone(phone), address(address) {}

int Customer::getId() const { return id; }
std::string Customer::getName() const { return name; }
std::string Customer::getEmail() const { return email; }
std::string Customer::getPhone() const { return phone; }
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

void Customer::display() const {
    std::cout << "Customer #" << id << " " << name
               << " | Email: " << email
               << " | Phone: " << phone
               << " | Address: " << address << "\n";
}

bool Customer::isValidEmail(const std::string& email) {
    return email.find('@') != std::string::npos;
}

bool Customer::isValidPhone(const std::string& phone) {
    if (phone.size() != 10) return false;
    return std::all_of(phone.begin(), phone.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
}
