#pragma once
#include <string>

// Encapsulation: all fields are private; the only way in/out is through
// getters and setters that validate input (email, phone) before accepting it.
class Customer {
private:
    int id;
    std::string name;
    std::string email;
    std::string phone;
    std::string address;

public:
    Customer(int id, const std::string& name, const std::string& email,
              const std::string& phone, const std::string& address);

    int getId() const;
    std::string getName() const;
    std::string getEmail() const;
    std::string getPhone() const;
    std::string getAddress() const;

    // Setters return false (and leave the field unchanged) if validation fails.
    void setName(const std::string& newName);
    bool setEmail(const std::string& newEmail);     // must contain '@'
    bool setPhone(const std::string& newPhone);      // must be exactly 10 digits
    void setAddress(const std::string& newAddress);

    void display() const;

    static bool isValidEmail(const std::string& email);
    static bool isValidPhone(const std::string& phone);
};
