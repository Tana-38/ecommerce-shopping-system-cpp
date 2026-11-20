#pragma once
#include <string>
#include <vector>

#include "ui/Color.h"

namespace shop {
namespace ui {

struct Bar {
    std::string label;
    double value;
    std::string valueText;
    RGB color;
};

// Horizontal bar chart with 1/8-cell precision and gradient bars.
// `progress` (0..1) lets the caller animate the bars growing.
std::vector<std::string> barChart(const std::vector<Bar>& bars, int labelWidth, int barWidth,
                                  double progress = 1.0);

// A gradient progress/meter bar:  ▕██████░░░░▏
std::string meter(double fraction, int width, const RGB& from, const RGB& to);

// One stacked bar showing each part's share, plus a legend line.
std::vector<std::string> shareBar(const std::vector<Bar>& parts, int width);

// Star rating such as ★★★★☆ (rounded to the nearest whole star).
std::string stars(double rating);

}  // namespace ui
}  // namespace shop
