#pragma once
#include <string>
#include <vector>

namespace shop {
namespace ui {

struct MenuItem {
    std::string label;
    std::string hint;      // dim text on the right
    bool enabled;
    MenuItem(const std::string& l, const std::string& h = "", bool e = true)
        : label(l), hint(h), enabled(e) {}
};

// Vertical menu drawn in a rounded box, driven by arrow keys / j k + Enter,
// or by pressing the item's number. Esc, q or b returns -1 ("back").
class Menu {
public:
    Menu(const std::string& title, const std::vector<MenuItem>& items, int width = 52);

    int run(int initial = 0) const;                    // interactive selection
    std::vector<std::string> lines(int selected) const;
    // Demo helper: draws the menu and walks the highlight down to `target`.
    void animateTo(int target) const;

    std::size_t size() const { return items_.size(); }

private:
    std::string title_;
    std::vector<MenuItem> items_;
    int width_;

    void draw(int selected, bool redraw) const;
    int nextEnabled(int from, int step) const;
};

}  // namespace ui
}  // namespace shop
