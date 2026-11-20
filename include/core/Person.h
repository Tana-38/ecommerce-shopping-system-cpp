#pragma once
#include <string>

namespace shop {

// OOP: abstraction -- Person is an abstract base for everyone who can log in.
// It owns the identity fields and password check that Customer and Admin share,
// and leaves role-specific behaviour as pure virtual functions.
class Person {
protected:
    int id;
    std::string name;
    std::string email;
    std::string phone;
    std::string passwordHash;

public:
    // OOP: constructors -- default + parameterised.
    Person();
    Person(int id, const std::string& name, const std::string& email,
           const std::string& phone, const std::string& passwordHash);
    // OOP: destructor -- virtual so a Person* can safely own a Customer/Admin.
    virtual ~Person();

    // OOP: abstraction -- pure virtual interface.
    virtual std::string getRole() const = 0;
    virtual void display() const = 0;

    // Greeting is virtual but has a sensible default.
    virtual std::string greeting() const;

    int getId() const;
    std::string getName() const;
    std::string getEmail() const;
    std::string getPhone() const;
    std::string firstName() const;

    bool checkPassword(const std::string& plain) const;
    void setPassword(const std::string& plain);   // stores the hash only
    const std::string& getPasswordHash() const { return passwordHash; }
};

} // namespace shop
