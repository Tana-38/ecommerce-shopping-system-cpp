#pragma once
#include <string>
#include <vector>

#include "ui/Color.h"

namespace shop {
namespace ui {

enum class Align { Left, Right, Center };

struct Column {
    std::string title;
    int width;
    Align align;
};

// Box-drawn table with rounded corners, coloured header and an optional
// highlighted (selected) row. Cells may contain ANSI colours.
class Table {
public:
    explicit Table(const std::vector<Column>& columns);

    Table& addRow(const std::vector<std::string>& cells);
    Table& addSeparator();
    void setSelected(int rowIndex) { selected_ = rowIndex; }
    void setBorderColor(const RGB& c) { border_ = c; }
    void setHeaderColor(const RGB& c) { header_ = c; }

    std::vector<std::string> lines() const;
    std::string render(int indent = 2) const;
    int width() const;

private:
    std::vector<Column> columns_;
    std::vector<std::vector<std::string>> rows_;   // empty vector = separator
    int selected_;
    RGB border_;
    RGB header_;

    std::string rule(const std::string& left, const std::string& mid,
                     const std::string& right) const;
    std::string formatCell(const std::string& text, const Column& col) const;
};

// Rounded box around content lines. innerWidth excludes the borders/padding.
std::vector<std::string> boxLines(const std::vector<std::string>& content, int innerWidth,
                                  const std::string& title, const RGB& border);
std::string box(const std::vector<std::string>& content, int innerWidth,
                const std::string& title, const RGB& border, int indent = 2);
// Places several blocks of lines next to each other.
std::vector<std::string> sideBySide(const std::vector<std::vector<std::string>>& blocks, int gap);
std::string indentLines(const std::vector<std::string>& lines, int indent);

}  // namespace ui
}  // namespace shop
