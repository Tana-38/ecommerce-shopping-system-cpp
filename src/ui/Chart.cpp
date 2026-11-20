#include "ui/Chart.h"

#include <algorithm>

#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {

namespace {
// U+258F..U+2589: one to seven eighths of a block.
const char* const kEighths[8] = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};

std::string solidBar(int eighths, int width, const RGB& from, const RGB& to) {
    std::string out;
    int full = eighths / 8;
    int part = eighths % 8;
    for (int i = 0; i < full; ++i) {
        double t = width > 1 ? static_cast<double>(i) / (width - 1) : 0.0;
        out += style::fg(style::lerp(from, to, t)) + "█";
    }
    if (part > 0) {
        double t = width > 1 ? static_cast<double>(full) / (width - 1) : 0.0;
        out += style::fg(style::lerp(from, to, t)) + kEighths[part];
    }
    return out + style::reset();
}
}  // namespace

std::vector<std::string> barChart(const std::vector<Bar>& bars, int labelWidth, int barWidth,
                                  double progress) {
    std::vector<std::string> out;
    double maxValue = 0;
    for (const Bar& b : bars) maxValue = std::max(maxValue, b.value);
    if (progress < 0) progress = 0;
    if (progress > 1) progress = 1;
    for (const Bar& b : bars) {
        double fraction = maxValue > 0 ? b.value / maxValue : 0.0;
        int eighths = static_cast<int>(fraction * progress * barWidth * 8 + 0.5);
        if (b.value > 0 && eighths == 0 && progress >= 1.0) eighths = 1;
        RGB light = style::lerp(b.color, RGB{255, 255, 255}, 0.35);
        std::string bar = solidBar(eighths, barWidth, b.color, light);
        int used = (eighths + 7) / 8;
        std::string line = style::paint(util::padRight(util::truncate(b.label, labelWidth), labelWidth),
                                        theme::kText) +
                           " " + style::paint("┤", theme::kFaint) + bar +
                           std::string(static_cast<std::size_t>(std::max(0, barWidth - used)), ' ') +
                           " " + style::paintBold(b.valueText, b.color);
        out.push_back(line);
    }
    return out;
}

std::string meter(double fraction, int width, const RGB& from, const RGB& to) {
    if (fraction < 0) fraction = 0;
    if (fraction > 1) fraction = 1;
    int eighths = static_cast<int>(fraction * width * 8 + 0.5);
    int used = (eighths + 7) / 8;
    std::string rest = util::repeat("░", std::max(0, width - used));
    return style::paint("▕", theme::kFaint) + solidBar(eighths, width, from, to) +
           style::paint(rest, theme::kFaint) + style::paint("▏", theme::kFaint);
}

std::vector<std::string> shareBar(const std::vector<Bar>& parts, int width) {
    double total = 0;
    for (const Bar& p : parts) total += p.value;
    std::string bar;
    std::string legend;
    int used = 0;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const Bar& p = parts[i];
        int cells = total > 0 ? static_cast<int>(p.value / total * width + 0.5) : 0;
        if (i + 1 == parts.size()) cells = std::max(0, width - used);   // fill exactly
        if (used + cells > width) cells = width - used;
        // Without colour, use different shades so the parts stay distinguishable.
        static const char* const shades[4] = {"█", "▓", "▒", "░"};
        std::string glyph = Terminal::colorEnabled() ? "█" : shades[i % 4];
        bar += style::fg(p.color) + util::repeat(glyph, cells) + style::reset();
        used += cells;
        int pct = total > 0 ? static_cast<int>(p.value * 100.0 / total + 0.5) : 0;
        if (!legend.empty()) legend += "   ";
        legend += style::paint(Terminal::colorEnabled() ? "●" : shades[i % 4], p.color) + " " +
                  p.label + " " + style::muted(util::toStr(pct) + "%");
    }
    return {bar, legend};
}

std::string stars(double rating) {
    int full = static_cast<int>(rating + 0.5);
    if (full > 5) full = 5;
    if (full < 0) full = 0;
    return style::paint(util::repeat("★", full), theme::kGold) +
           style::paint(util::repeat("☆", 5 - full), theme::kFaint);
}

}  // namespace ui
}  // namespace shop
