#pragma once
#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "util/Exceptions.h"

namespace shop {

// OOP: templates -- a generic in-memory repository for any entity type T
// that exposes `int getId() const`. The store keeps one Repository for each
// of Product, Customer and Order, so lookup/sort/filter code is written once.
// OOP: smart pointers -- items are held as std::shared_ptr<T>, so a product
// can live in the catalog and in several carts at the same time safely.
template <typename T>
class Repository {
public:
    using Ptr = std::shared_ptr<T>;

    // Adds an item; throws InvalidInputException if the ID is already taken.
    void add(const Ptr& item) {
        if (!item) throw InvalidInputException("Cannot add an empty item.");
        if (findById(item->getId())) {
            throw InvalidInputException("Duplicate ID " + idText(item->getId()) + ".");
        }
        items_.push_back(item);
    }

    // Removes by ID; returns false if no such item.
    bool remove(int id) {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [id](const Ptr& p) { return p->getId() == id; });
        if (it == items_.end()) return false;
        items_.erase(it);
        return true;
    }

    // Returns nullptr when not found.
    Ptr findById(int id) const {
        // OOP: STL algorithms -- find_if with a capturing lambda.
        auto it = std::find_if(items_.begin(), items_.end(),
                               [id](const Ptr& p) { return p->getId() == id; });
        return it == items_.end() ? Ptr() : *it;
    }

    // Like findById but throws NotFoundException.
    Ptr getById(int id) const {
        Ptr p = findById(id);
        if (!p) throw NotFoundException("No record with ID " + idText(id) + ".");
        return p;
    }

    // OOP: templates -- member function templates accept any predicate / key.
    template <typename Pred>
    std::vector<Ptr> findAll(Pred pred) const {
        std::vector<Ptr> out;
        std::copy_if(items_.begin(), items_.end(), std::back_inserter(out),
                     [&](const Ptr& p) { return pred(*p); });
        return out;
    }

    template <typename Pred>
    Ptr findFirst(Pred pred) const {
        auto it = std::find_if(items_.begin(), items_.end(),
                               [&](const Ptr& p) { return pred(*p); });
        return it == items_.end() ? Ptr() : *it;
    }

    template <typename Pred>
    std::size_t countIf(Pred pred) const {
        return static_cast<std::size_t>(std::count_if(
            items_.begin(), items_.end(), [&](const Ptr& p) { return pred(*p); }));
    }

    template <typename KeyFn>
    std::vector<Ptr> sortedBy(KeyFn key, bool ascending = true) const {
        std::vector<Ptr> out = items_;
        std::stable_sort(out.begin(), out.end(), [&](const Ptr& a, const Ptr& b) {
            return ascending ? key(*a) < key(*b) : key(*b) < key(*a);
        });
        return out;
    }

    int maxId() const {
        int best = 0;
        for (const Ptr& p : items_) best = std::max(best, p->getId());
        return best;
    }

    const std::vector<Ptr>& all() const { return items_; }
    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    void clear() { items_.clear(); }

    typename std::vector<Ptr>::const_iterator begin() const { return items_.begin(); }
    typename std::vector<Ptr>::const_iterator end() const { return items_.end(); }

private:
    std::vector<Ptr> items_;   // OOP: STL containers -- std::vector

    static std::string idText(int id) {
        std::string s;
        long long v = id < 0 ? -static_cast<long long>(id) : id;
        do { s.insert(s.begin(), static_cast<char>('0' + v % 10)); v /= 10; } while (v);
        return id < 0 ? "-" + s : s;
    }
};

// OOP: templates -- free function templates that work on any container of
// (smart) pointers whose element type has getId().
template <typename Container>
auto findById(const Container& items, int id) -> typename Container::value_type {
    for (const auto& p : items) {
        if (p->getId() == id) return p;
    }
    return typename Container::value_type();
}

template <typename T, typename KeyFn>
void sortBy(std::vector<T>& items, KeyFn key, bool ascending = true) {
    std::stable_sort(items.begin(), items.end(), [&](const T& a, const T& b) {
        return ascending ? key(a) < key(b) : key(b) < key(a);
    });
}

} // namespace shop
