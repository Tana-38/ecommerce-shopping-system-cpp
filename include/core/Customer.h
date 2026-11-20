#pragma once
#include <string>

#include "core/Person.h"
#include "util/Money.h"

namespace shop {

// Encapsulation: all fields are private; the only way in/out is through
// getters and setters that validate input (email, phone) before accepting it.
// OOP: inheritance (single) -- Customer IS-A Person.
// OOP: encapsulation -- private state + validated setters.
class Customer : public Person {
private:
    std::string address;
    Money wallet;            // ShopSphere wallet balance
    std::string joinedOn;    // "2026-08-14"
    static int nextId_;

public:
    Customer(int id, const std::string& name, const std::string& email,
              const std::string& phone, const std::string& address);
    // OOP: function overloading -- extended constructor used by the store/CSV loader.
    Customer(int id, const std::string& name, const std::string& email,
             const std::string& phone, const std::string& address,
             const std::string& passwordHash, const Money& wallet,
             const std::string& joinedOn);

    // Getters inherited from Person: getId, getName, getEmail, getPhone.
    std::string getAddress() const;
    const Money& getWallet() const { return wallet; }
    const std::string& getJoinedOn() const { return joinedOn; }
    bool isGuest() const { return id == 0; }

    // Setters return false (and leave the field unchanged) if validation fails.
    void setName(const std::string& newName);
    bool setEmail(const std::string& newEmail);     // must look like name@domain.tld
    bool setPhone(const std::string& newPhone);      // must be exactly 10 digits
    void setAddress(const std::string& newAddress);

    // Wallet operations (throw on invalid amounts / insufficient balance).
    void creditWallet(const Money& amount);
    void debitWallet(const Money& amount);

    // OOP: runtime polymorphism -- Person's pure virtuals implemented here.
    std::string getRole() const override { return isGuest() ? "Guest" : "Customer"; }
    void display() const override;

    static bool isValidEmail(const std::string& email);
    static bool isValidPhone(const std::string& phone);

    // OOP: static members -- customer ID generator.
    static int generateId();
    static void observeId(int usedId);
};

} // namespace shop
