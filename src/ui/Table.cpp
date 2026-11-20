#include "ui/Table.h"

#include <algorithm>

#include "util/StringUtil.h"

namespace shop {
namespace ui {

using util::displayWidth;
using util::repeat;

Table::Table(const std::vector<Column>& columns)
    : columns_(columns), selected_(-1), border_(theme::kFaint), header_(theme::kSecondary) {}

Table& Table::addRow(const std::vector<std::string>& cells) {
    rows_.push_back(cells);
    if (rows_.back().empty()) rows_.back().push_back("");
    return *this;
}

Table& Table::addSeparator() {
    rows_.push_back(std::vector<std::string>());
    return *this;
}

int Table::width() const {
    int w = 1;
    for (const Column& c : columns_) w += c.width + 3;
    return w;
}

std::string Table::rule(const std::string& left, const std::string& mid,
                        const std::string& right) const {
    std::string s = left;
    for (std::size_t i = 0; i < columns_.size(); ++i) {
        s += repeat("─", columns_[i].width + 2);
        s += (i + 1 < columns_.size()) ? mid : right;
    }
    return style::paint(s, border_);
}

std::string Table::formatCell(const std::string& text, const Column& col) const {
    std::string t = text;
    if (displayWidth(t) > col.width && t.find('\x1b') == std::string::npos) {
        t = util::truncate(t, col.width);
    }
    switch (col.align) {
        case Align::Right: return util::padLeft(t, col.width);
        case Align::Center: return util::center(t, col.width);
        case Align::Left:
        default: return util::padRight(t, col.width);
    }
}

std::vector<std::string> Table::lines() const {
    // Every line starts with a one-column gutter; the selected row puts a
    // pointer there so the selection is visible even without colours.
    std::vector<std::string> out;
    std::string bar = style::paint("│", border_);
    out.push_back(" " + rule("╭", "┬", "╮"));

    std::string head = " " + bar;
    for (const Column& c : columns_) {
        head += " " + style::paintBold(formatCell(c.title, c), header_) + " " + bar;
    }
    out.push_back(head);
    out.push_back(" " + rule("├", "┼", "┤"));

    int dataIndex = 0;
    for (const auto& row : rows_) {
        if (row.empty()) {
            out.push_back(" " + rule("├", "┼", "┤"));
            continue;
        }
        bool sel = dataIndex == selected_;
        std::string line = sel ? style::paintBold("▶", theme::kAccent) + bar : " " + bar;
        for (std::size_t i = 0; i < columns_.size(); ++i) {
            std::string cell = formatCell(i < row.size() ? row[i] : "", columns_[i]);
            if (sel) {
                // Keep the highlight background alive across inner colour resets.
                std::string bgOn = style::bg(theme::kSelect) + style::bold();
                std::string body = util::replaceAll(cell, "\x1b[0m", "\x1b[0m" + bgOn);
                line += bgOn + " " + body + " " + style::reset() + bar;
            } else {
                line += " " + cell + " " + bar;
            }
        }
        out.push_back(line);
        ++dataIndex;
    }
    out.push_back(" " + rule("╰", "┴", "╯"));
    return out;
}

std::string Table::render(int indent) const {
    return indentLines(lines(), indent > 0 ? indent - 1 : 0);
}

std::vector<std::string> boxLines(const std::vector<std::string>& content, int innerWidth,
                                  const std::string& title, const RGB& border) {
    std::vector<std::string> out;
    std::string top = "╭─";
    int used = 1;
    if (!title.empty()) {
        std::string t = " " + title + " ";
        top = style::paint("╭─", border) + style::paintBold(t, theme::kText);
        used += displayWidth(t);
        top += style::paint(repeat("─", std::max(0, innerWidth + 2 - used)) + "╮", border);
    } else {
        top = style::paint("╭" + repeat("─", innerWidth + 2) + "╮", border);
    }
    out.push_back(top);
    std::string bar = style::paint("│", border);
    for (const std::string& line : content) {
        out.push_back(bar + " " + util::padRight(line, innerWidth) + " " + bar);
    }
    out.push_back(style::paint("╰" + repeat("─", innerWidth + 2) + "╯", border));
    return out;
}

std::string box(const std::vector<std::string>& content, int innerWidth, const std::string& title,
                const RGB& border, int indent) {
    return indentLines(boxLines(content, innerWidth, title, border), indent);
}

std::vector<std::string> sideBySide(const std::vector<std::vector<std::string>>& blocks, int gap) {
    std::size_t rows = 0;
    std::vector<int> widths;
    for (const auto& b : blocks) {
        rows = std::max(rows, b.size());
        int w = 0;
        for (const auto& l : b) w = std::max(w, displayWidth(l));
        widths.push_back(w);
    }
    std::vector<std::string> out(rows);
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t b = 0; b < blocks.size(); ++b) {
            std::string cell = r < blocks[b].size() ? blocks[b][r] : "";
            out[r] += util::padRight(cell, widths[b]);
            if (b + 1 < blocks.size()) out[r] += std::string(static_cast<std::size_t>(gap), ' ');
        }
    }
    return out;
}

std::string indentLines(const std::vector<std::string>& lines, int indent) {
    std::string pad(static_cast<std::size_t>(std::max(0, indent)), ' ');
    std::string out;
    for (const std::string& l : lines) out += pad + l + "\n";
    return out;
}

}  // namespace ui
}  // namespace shop
