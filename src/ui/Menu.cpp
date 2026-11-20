#include "ui/Menu.h"

#include "ui/Color.h"
#include "ui/Table.h"
#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {

Menu::Menu(const std::string& title, const std::vector<MenuItem>& items, int width)
    : title_(title), items_(items), width_(width) {}

std::vector<std::string> Menu::lines(int selected) const {
    std::vector<std::string> content;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        const MenuItem& item = items_[i];
        bool sel = static_cast<int>(i) == selected;
        std::string number = util::toStr(i + 1);
        std::string left;
        if (sel) {
            left = style::paintBold("❯ ", theme::kAccent) +
                   style::badge(number, theme::kPrimary, RGB{255, 255, 255}) + " " +
                   style::paintBold(item.label, RGB{255, 255, 255});
        } else if (!item.enabled) {
            left = "  " + style::paint(" " + number + " ", theme::kFaint) + " " +
                   style::paint(item.label, theme::kFaint);
        } else {
            left = "  " + style::paint(" " + number + " ", theme::kMuted) + " " +
                   style::paint(item.label, theme::kText);
        }
        std::string hint = item.hint.empty() ? "" : style::muted(item.hint);
        int gap = width_ - util::displayWidth(left) - util::displayWidth(hint);
        content.push_back(left + std::string(static_cast<std::size_t>(gap > 1 ? gap : 1), ' ') + hint);
    }
    return boxLines(content, width_, title_, theme::kPrimary);
}

void Menu::draw(int selected, bool redraw) const {
    std::vector<std::string> ls = lines(selected);
    if (redraw && Terminal::vtEnabled()) Terminal::cursorUp(static_cast<int>(ls.size()));
    std::string out;
    for (const std::string& l : ls) {
        if (Terminal::vtEnabled()) out += "\x1b[2K";
        out += "  " + l + "\n";
    }
    Terminal::write(out);
    Terminal::flush();
}

int Menu::nextEnabled(int from, int step) const {
    int n = static_cast<int>(items_.size());
    int i = from;
    for (int tries = 0; tries < n; ++tries) {
        i = (i + step + n) % n;
        if (items_[static_cast<std::size_t>(i)].enabled) return i;
    }
    return from;
}

int Menu::run(int initial) const {
    if (items_.empty()) return -1;
    int selected = initial;
    if (selected < 0 || selected >= static_cast<int>(items_.size()) ||
        !items_[static_cast<std::size_t>(selected)].enabled) {
        selected = nextEnabled(-1, 1);   // first enabled item
    }
    bool live = Terminal::interactive() && Terminal::vtEnabled();
    if (live) Terminal::hideCursor();
    draw(selected, false);
    if (!live) {
        Terminal::write("  " + style::muted("Choose 1-" + util::toStr(items_.size()) +
                                            " (Esc/q = back): "));
    }
    for (;;) {
        Key k = Terminal::readKey();
        int chosen = -2;
        switch (k.code) {
            case KeyCode::Up: selected = nextEnabled(selected, -1); break;
            case KeyCode::Down:
            case KeyCode::Tab: selected = nextEnabled(selected, 1); break;
            case KeyCode::Home: selected = nextEnabled(-1, 1); break;
            case KeyCode::End: selected = nextEnabled(static_cast<int>(items_.size()), -1); break;
            case KeyCode::Enter: chosen = selected; break;
            case KeyCode::Escape: chosen = -1; break;
            case KeyCode::Char:
                if (k.ch >= '1' && k.ch <= '9') {
                    int idx = k.ch - '1';
                    if (idx < static_cast<int>(items_.size()) &&
                        items_[static_cast<std::size_t>(idx)].enabled) {
                        selected = idx;
                        chosen = idx;
                    }
                } else if (k.is('k')) {
                    selected = nextEnabled(selected, -1);
                } else if (k.is('j')) {
                    selected = nextEnabled(selected, 1);
                } else if (k.is('q') || k.is('b')) {
                    chosen = -1;
                }
                break;
            default: break;
        }
        if (live) draw(selected, true);
        if (chosen != -2) {
            if (live) Terminal::showCursor();
            if (!live) Terminal::write("\n");
            return chosen;
        }
    }
}

void Menu::animateTo(int target) const {
    if (!Terminal::animationEnabled()) {
        draw(target, false);
        return;
    }
    draw(0, false);
    for (int i = 1; i <= target; ++i) {
        Terminal::sleepMs(260);
        draw(i, true);
    }
    Terminal::sleepMs(380);
}

}  // namespace ui
}  // namespace shop
