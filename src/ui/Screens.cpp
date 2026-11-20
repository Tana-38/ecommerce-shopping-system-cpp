#include "ui/Screens.h"

#include <algorithm>
#include <cmath>

#include "payment/Coupon.h"
#include "ui/Animation.h"
#include "ui/Chart.h"
#include "ui/Color.h"
#include "ui/Table.h"
#include "ui/Terminal.h"
#include "util/StringUtil.h"

namespace shop {
namespace ui {
namespace screens {

using util::displayWidth;
using util::padLeft;
using util::padRight;
using util::toStr;

namespace {

const RGB kWhite{255, 255, 255};

std::string spaces(int n) { return std::string(static_cast<std::size_t>(n > 0 ? n : 0), ' '); }

// Paints `content` on a solid background across `width` columns, keeping the
// background alive through any colour resets inside the content.
std::string onBackground(const std::string& content, const RGB& background, int width) {
    if (!Terminal::colorEnabled()) return padRight(content, width);
    std::string bgOn = style::bg(background);
    std::string body = util::replaceAll(content, "\x1b[0m", "\x1b[0m" + bgOn);
    return bgOn + padRight(body, width) + style::reset();
}

// Two-column "label  value" row padded to `width`.
std::string labelValue(const std::string& label, const std::string& value, int width) {
    int gap = width - displayWidth(label) - displayWidth(value);
    return label + spaces(gap > 1 ? gap : 1) + value;
}

// ANSI Shadow style glyphs, 6 rows each.
const char* const kGlyphS[6] = {"███████╗", "██╔════╝", "███████╗", "╚════██║", "███████║", "╚══════╝"};
const char* const kGlyphH[6] = {"██╗  ██╗", "██║  ██║", "███████║", "██╔══██║", "██║  ██║", "╚═╝  ╚═╝"};
const char* const kGlyphO[6] = {" ██████╗ ", "██╔═══██╗", "██║   ██║", "██║   ██║", "╚██████╔╝", " ╚═════╝ "};
const char* const kGlyphP[6] = {"██████╗ ", "██╔══██╗", "██████╔╝", "██╔═══╝ ", "██║     ", "╚═╝     "};
const char* const kGlyphE[6] = {"███████╗", "██╔════╝", "█████╗  ", "██╔══╝  ", "███████╗", "╚══════╝"};
const char* const kGlyphR[6] = {"██████╗ ", "██╔══██╗", "██████╔╝", "██╔══██╗", "██║  ██║", "╚═╝  ╚═╝"};

RGB cyclic(double t) {
    t = t - std::floor(t);
    if (t < 1.0 / 3) return style::lerp(theme::kPrimary, theme::kSecondary, t * 3);
    if (t < 2.0 / 3) return style::lerp(theme::kSecondary, theme::kAccent, (t - 1.0 / 3) * 3);
    return style::lerp(theme::kAccent, theme::kPrimary, (t - 2.0 / 3) * 3);
}

std::string money(const Money& m) { return m.shortStr(); }

}  // namespace

// ============================================================================
// Chrome
// ============================================================================

std::vector<std::string> logoLines(double phase) {
    const char* const* word[10] = {kGlyphS, kGlyphH, kGlyphO, kGlyphP, kGlyphS,
                                   kGlyphP, kGlyphH, kGlyphE, kGlyphR, kGlyphE};
    std::vector<std::string> out;
    for (int row = 0; row < 6; ++row) {
        std::string plain;
        for (int g = 0; g < 10; ++g) plain += word[g][row];
        if (!Terminal::colorEnabled()) {
            out.push_back(plain);
            continue;
        }
        // Colour each code point; the ╗╝═ "shadow" strokes are dimmer.
        std::string line;
        int total = displayWidth(plain);
        int col = 0;
        for (std::size_t i = 0; i < plain.size();) {
            unsigned char c = static_cast<unsigned char>(plain[i]);
            std::size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
            std::string cp = plain.substr(i, len);
            if (cp != " ") {
                RGB colour = cyclic(static_cast<double>(col) / total * 0.9 + row * 0.025 + phase);
                if (cp != "█") colour = style::lerp(colour, theme::kPanel, 0.45);
                line += style::fg(colour) + cp;
            } else {
                line += " ";
            }
            ++col;
            i += len;
        }
        out.push_back(line + style::reset());
    }
    return out;
}

void splash(bool showLoading) {
    Terminal::clear();
    Terminal::hideCursor();
    Terminal::write("\n\n");
    const int indent = 2 + (kPageWidth - 81) / 2;
    if (Terminal::animationEnabled()) {
        for (int frame = 0; frame < 22; ++frame) {
            if (frame > 0) Terminal::cursorUp(6);
            Terminal::write(indentLines(logoLines(frame * 0.045), indent));
            Terminal::flush();
            Terminal::sleepMs(40);
        }
    } else {
        Terminal::write(indentLines(logoLines(0.0), indent));
    }
    Terminal::write("\n");
    std::string tagline = "Shop smarter.  Live better.";
    anim::typewriter(style::paintBold(tagline, kWhite), 28,
                     2 + (kPageWidth - displayWidth(tagline)) / 2);
    std::string sub = "C++ object-oriented console store  ·  Group 17  ·  VIT Pune";
    Terminal::write(spaces(2 + (kPageWidth - displayWidth(sub)) / 2) + style::muted(sub) + "\n\n");
    if (showLoading) {
        const int barIndent = 2 + (kPageWidth - 62) / 2;
        anim::progressBar("Loading catalog", 700, 36, barIndent);
        anim::progressBar("Securing payments", 500, 36, barIndent);
        Terminal::write("\n");
    }
    Terminal::showCursor();
    Terminal::flush();
}

std::string header(const std::string& crumb, const std::string& right) {
    std::string brand = Terminal::colorEnabled()
                            ? style::bold() + style::gradient("◆ ShopSphere", theme::kSecondary,
                                                              theme::kAccent)
                            : "◆ ShopSphere";
    std::string left = " " + brand + style::paint("  ›  ", theme::kFaint) +
                       style::paintBold(crumb, kWhite);
    std::string r = style::paint(right, theme::kText) + " ";
    int gap = kPageWidth - displayWidth(left) - displayWidth(r);
    std::string line = onBackground(left + spaces(gap) + r, theme::kPanel, kPageWidth);

    std::string rule;
    if (Terminal::colorEnabled()) {
        for (int i = 0; i < kPageWidth; ++i) {
            rule += style::fg(style::lerp3(theme::kPrimary, theme::kSecondary, theme::kAccent,
                                           static_cast<double>(i) / (kPageWidth - 1))) +
                    "▀";
        }
        rule += style::reset();
    } else {
        rule = util::repeat("═", kPageWidth);
    }
    return "  " + line + "\n  " + rule + "\n";
}

std::string footer(const KeyHints& keys) {
    std::string s = " ";
    for (const auto& k : keys) {
        s += style::badge(k.first, theme::kFaint, kWhite) + " " + style::paint(k.second, theme::kMuted) +
             "   ";
    }
    return "\n  " + style::paint(util::repeat("─", kPageWidth), theme::kFaint) + "\n  " + s + "\n";
}

void begin(const std::string& crumb, const std::string& right) {
    Terminal::clear();
    Terminal::write(header(crumb, right));
    Terminal::write("\n");
}

void sectionTitle(const std::string& title, const std::string& subtitle) {
    std::string t = Terminal::colorEnabled()
                        ? style::bold() + style::gradient(title, theme::kSecondary, theme::kPrimary)
                        : title;
    Terminal::write("  " + t);
    if (!subtitle.empty()) Terminal::write("   " + style::muted(subtitle));
    Terminal::write("\n\n");
}

// ============================================================================
// Feedback & input
// ============================================================================

void notify(Tone tone, const std::string& text) {
    std::string icon;
    RGB c;
    switch (tone) {
        case Tone::Success: icon = "✔"; c = theme::kSuccess; break;
        case Tone::Error: icon = "✘"; c = theme::kDanger; break;
        case Tone::Warning: icon = "!"; c = theme::kWarning; break;
        case Tone::Info:
        default: icon = "i"; c = theme::kInfo; break;
    }
    Terminal::write("  " + style::paintBold(" " + icon + " ", c) + style::paint(text, c) + "\n");
    Terminal::flush();
}

void showError(const ShopException& e) {
    notify(Tone::Error, e.title() + ": " + e.what());   // OOP: runtime polymorphism (title())
}

void waitKey(const std::string& text) {
    if (!Terminal::interactive()) {
        Terminal::write("\n");
        return;
    }
    Terminal::write("\n  " + style::muted("› " + text + " "));
    Terminal::hideCursor();
    Terminal::readKey();
    Terminal::showCursor();
    Terminal::write("\n");
}

std::string prompt(const std::string& label, const std::string& hint) {
    Terminal::showCursor();
    Terminal::write("  " + style::paintBold("❯ ", theme::kAccent) + style::paint(label, theme::kText));
    if (!hint.empty()) Terminal::write(" " + style::muted("(" + hint + ")"));
    Terminal::write(style::paint(": ", theme::kText) + style::fg(theme::kSecondary));
    std::string line = Terminal::readLine();
    Terminal::write(style::reset());
    return util::trim(line);
}

std::string promptSecret(const std::string& label) {
    Terminal::showCursor();
    Terminal::write("  " + style::paintBold("❯ ", theme::kAccent) + style::paint(label, theme::kText) +
                    style::paint(": ", theme::kText));
    return Terminal::readSecret();
}

bool confirm(const std::string& question) {
    Terminal::write("  " + style::paintBold("? ", theme::kWarning) + style::paint(question, theme::kText) +
                    style::muted(" [y/n] "));
    for (;;) {
        Key k = Terminal::readKey();
        if (k.is('y')) {
            Terminal::write(style::paint("yes", theme::kSuccess) + "\n");
            return true;
        }
        if (k.is('n') || k.code == KeyCode::Escape) {
            Terminal::write(style::paint("no", theme::kDanger) + "\n");
            return false;
        }
    }
}

void fakeInput(const std::string& label, const std::string& value, bool secret) {
    Terminal::write("  " + style::paintBold("❯ ", theme::kAccent) + style::paint(label, theme::kText) +
                    style::paint(": ", theme::kText));
    Terminal::flush();
    Terminal::sleepMs(250);
    std::string shown = secret ? std::string(value.size(), '*') : value;
    anim::typewriter(style::paint(shown, theme::kSecondary), 55, 0);
    Terminal::sleepMs(200);
}

// ============================================================================
// Building blocks
// ============================================================================

std::string categoryTag(const std::string& category) {
    RGB c = theme::forCategory(category);
    return style::paint("▌", c) + style::paint(category, c);
}

std::string statusBadge(OrderStatus status) {
    switch (status) {
        case OrderStatus::Placed: return style::badge(" Placed  ", theme::kInfo, kWhite);
        case OrderStatus::Shipped: return style::badge(" Shipped ", theme::kWarning, theme::kBlack);
        case OrderStatus::Delivered: return style::badge("Delivered", theme::kSuccess, theme::kBlack);
        case OrderStatus::Cancelled: return style::badge("Cancelled", theme::kDanger, kWhite);
    }
    return "";
}

std::string stockText(int stock) {
    if (stock <= 0) return style::paintBold("Out", theme::kDanger);
    if (stock <= 5) return style::paintBold(toStr(stock) + "!", theme::kDanger);
    if (stock <= 10) return style::paint(toStr(stock), theme::kWarning);
    return style::paint(toStr(stock), theme::kSuccess);
}

std::string shortPayment(const std::string& method) {
    if (method == "Cash on Delivery") return "COD";
    if (method == "ShopSphere Wallet") return "Wallet";
    return method;
}

std::string filterBar(const std::string& category, SortKey sort, const std::string& search,
                      std::size_t count) {
    std::string s = "  " + style::muted("Category ");
    for (const std::string& c : Catalog::categories()) {
        if (c == category) {
            RGB col = c == "All" ? theme::kPrimary : theme::forCategory(c);
            s += style::badge(c, col, theme::kBlack) + " ";
        } else {
            s += style::paint(" " + c + " ", theme::kMuted) + " ";
        }
    }
    s += style::muted("  Sort ") + style::paintBold(Catalog::sortLabel(sort), theme::kSecondary);
    if (!search.empty()) {
        s += style::muted("  Search ") + style::paintBold("\"" + search + "\"", theme::kAccent);
    }
    s += style::muted("  · " + toStr(count) + (count == 1 ? " item" : " items"));
    return s + "\n";
}

std::string stepper(int step) {
    const char* names[3] = {"Address", "Payment", "Confirmation"};
    std::string s = "  ";
    for (int i = 0; i < 3; ++i) {
        if (i < step) {
            s += style::paintBold("✔ " + std::string(names[i]), theme::kSuccess);
        } else if (i == step) {
            s += style::badge(toStr(i + 1) + " " + names[i], theme::kPrimary, kWhite);
        } else {
            s += style::paint("○ " + std::string(names[i]), theme::kFaint);
        }
        if (i < 2) s += style::paint(i < step ? "  ━━━━━━  " : "  ──────  ", i < step ? theme::kSuccess
                                                                                     : theme::kFaint);
    }
    return s + "\n\n";
}

// ============================================================================
// Catalog / cart / orders
// ============================================================================

std::string productTable(const std::vector<ProductPtr>& items, int selected, int offset,
                         int pageSize) {
    Table t({{"ID", 3, Align::Right},
             {"Product", 23, Align::Left},
             {"Category", 12, Align::Left},
             {"MRP", 8, Align::Right},
             {"Price", 10, Align::Right},
             {"Off", 4, Align::Right},
             {"Rating", 5, Align::Left},
             {"Stock", 5, Align::Right}});
    int end = std::min(static_cast<int>(items.size()), offset + pageSize);
    for (int i = offset; i < end; ++i) {
        const Product& p = *items[static_cast<std::size_t>(i)];
        int pct = static_cast<int>(p.discountPercent() + 0.5);
        t.addRow({style::muted(toStr(p.getId())), util::truncate(p.getName(), 23),
                  categoryTag(p.getCategory()),
                  style::dim() + style::strike() + money(Money(p.getPrice())) + style::reset(),
                  style::paintBold(money(Money(p.getFinalPrice())), kWhite),
                  style::paint("-" + toStr(pct) + "%", theme::kAccent),
                  style::paint("★ ", theme::kGold) + util::fixed(p.getRating(), 1),
                  stockText(p.getStock())});
    }
    if (items.empty()) {
        t.addRow({"", style::muted("No products match."), "", "", "", "", "", ""});
    }
    t.setSelected(selected - offset);
    std::string out = t.render(2);
    if (static_cast<int>(items.size()) > pageSize) {
        out += "  " + style::muted("  Showing " + toStr(offset + 1) + "-" + toStr(end) + " of " +
                                   toStr(items.size()) + "   (↑↓ to scroll)") + "\n";
    }
    return out;
}

std::string productCard(const Product& p, int inCart) {
    const int w = 66;
    std::vector<std::string> c;
    std::string name = style::paintBold(p.getName(), kWhite);
    std::string rating = stars(p.getRating()) + " " + style::paint(util::fixed(p.getRating(), 1), theme::kGold);
    c.push_back(labelValue(name, rating, w));
    std::string typeLine = style::badge(util::toUpper(p.getCategory()),
                                        theme::forCategory(p.getCategory()), theme::kBlack);
    if (p.getType() != p.getCategory()) typeLine += "  " + style::muted(p.getType());
    c.push_back(typeLine);
    c.push_back("");
    Money mrp(p.getPrice());
    Money fin(p.getFinalPrice());
    Money save = mrp - fin;
    int pct = static_cast<int>(p.discountPercent() + 0.5);
    c.push_back(style::muted(padRight("MRP", 10)) + style::dim() + style::strike() + mrp.str() +
                style::reset());
    c.push_back(style::muted(padRight("Price", 10)) + style::paintBold(fin.str(), theme::kSuccess) +
                "   " + style::badge("SAVE " + toStr(pct) + "%", theme::kAccent, kWhite) + " " +
                style::paint("you save " + save.str(), theme::kAccent));
    c.push_back(style::muted(padRight("Offer", 10)) + style::paint(p.discountRule(), theme::kText));
    c.push_back(style::muted(padRight("Details", 10)) + style::paint(p.attributeSummary(), theme::kText));
    std::string stock;
    double fill = std::min(1.0, p.getStock() / 50.0);
    if (p.getStock() <= 0) {
        stock = style::paintBold("Out of stock", theme::kDanger);
    } else if (p.getStock() <= 5) {
        stock = meter(fill, 16, theme::kDanger, theme::kWarning) + " " +
                style::paintBold("Only " + toStr(p.getStock()) + " left - hurry!", theme::kDanger);
    } else {
        stock = meter(fill, 16, theme::kSuccess, theme::kSecondary) + " " +
                style::paint(toStr(p.getStock()) + " in stock", theme::kSuccess);
    }
    c.push_back(style::muted(padRight("Stock", 10)) + stock);
    if (inCart > 0) {
        c.push_back(style::muted(padRight("In cart", 10)) +
                    style::paintBold(toStr(inCart) + " unit(s)", theme::kSecondary));
    }
    return box(c, w, "Product #" + toStr(p.getId()), theme::forCategory(p.getCategory()), 2);
}

std::string cartTable(const Cart& cart, int selected) {
    Table t({{"#", 2, Align::Right},
             {"Product", 26, Align::Left},
             {"Category", 12, Align::Left},
             {"Unit", 10, Align::Right},
             {"Qty", 4, Align::Center},
             {"Saved", 9, Align::Right},
             {"Total", 11, Align::Right}});
    int n = 1;
    for (const CartItem& item : cart.items()) {
        t.addRow({style::muted(toStr(n++)), util::truncate(item.product()->getName(), 26),
                  categoryTag(item.product()->getCategory()), money(item.unitFinal()),
                  style::paintBold(toStr(item.quantity()), theme::kSecondary),
                  style::paint(money(item.lineDiscount()), theme::kAccent),
                  style::paintBold(money(item.lineTotal()), kWhite)});
    }
    if (cart.isEmpty()) {
        t.addRow({"", style::muted("Your cart is empty"), "", "", "", "", ""});
    }
    t.setSelected(selected);
    return t.render(2);
}

std::vector<std::string> breakdownLines(const PriceBreakdown& b, int width) {
    std::vector<std::string> l;
    l.push_back(labelValue(style::muted("MRP total (" + toStr(b.units) + " items)"),
                           style::paint(b.mrpTotal.str(), theme::kText), width));
    l.push_back(labelValue(style::muted("Product discounts"),
                           style::paint("-" + b.productDiscount.str(), theme::kSuccess), width));
    l.push_back(labelValue(style::muted("Subtotal"), style::paint(b.subtotal.str(), theme::kText), width));
    if (!b.couponCode.empty()) {
        if (b.couponDiscount.isZero()) {
            l.push_back(labelValue(style::muted("Coupon " + b.couponCode),
                                   style::paint("not eligible yet", theme::kWarning), width));
        } else {
            l.push_back(labelValue(style::muted("Coupon ") + style::paintBold(b.couponCode, theme::kAccent),
                                   style::paint("-" + b.couponDiscount.str(), theme::kSuccess), width));
        }
    }
    l.push_back(labelValue(style::muted("Delivery"),
                           b.deliveryFee.isZero() ? style::paintBold("FREE", theme::kSuccess)
                                                  : style::paint(b.deliveryFee.str(), theme::kText),
                           width));
    l.push_back(labelValue(style::muted("GST @ 5%"), style::paint(b.gst.str(), theme::kText), width));
    l.push_back(style::paint(util::repeat("─", width), theme::kFaint));
    l.push_back(labelValue(style::paintBold("Grand total", kWhite),
                           style::paintBold(b.total.str(), theme::kSecondary), width));
    if (!b.totalSavings().isZero()) {
        l.push_back(style::paint("You save " + b.totalSavings().str() + " on this order", theme::kSuccess));
    }
    return l;
}

std::string cartSummary(const Cart& cart) {
    PriceBreakdown b = cart.breakdown();
    std::vector<std::string> left = boxLines(breakdownLines(b, 40), 40, "Price details", theme::kSecondary);

    std::vector<std::string> offers;
    for (const auto& c : CouponBook::all()) {
        std::string status;
        if (cart.hasCoupon() && cart.coupon()->code() == c->code()) {
            status = style::paintBold("applied", theme::kSuccess);
        } else if (c->isApplicable(b.subtotal)) {
            status = style::paint("eligible", theme::kSecondary);
        } else {
            status = style::muted("min " + c->minOrder().shortStr());
        }
        offers.push_back(labelValue(style::paintBold(padRight(c->code(), 10), theme::kAccent) +
                                        style::paint(c->kind() == "Flat" ? "flat off" : "percent", theme::kMuted),
                                    status, 40));
        offers.push_back("  " + style::muted(util::truncate(c->description(), 38)));
    }
    offers.push_back("");
    Money threshold(pricing::kFreeDeliveryFrom);
    if (b.units == 0) {
        offers.push_back(style::muted("Free delivery on orders above " + threshold.shortStr()));
    } else if (b.subtotal < threshold) {
        offers.push_back(style::paint("Add " + (threshold - b.subtotal).str() + " for FREE delivery",
                                      theme::kWarning));
        offers.push_back(meter(b.subtotal.rupees() / threshold.rupees(), 30, theme::kWarning,
                               theme::kSuccess));
    } else {
        offers.push_back(style::paintBold("✔ You unlocked FREE delivery", theme::kSuccess));
        offers.push_back(meter(1.0, 30, theme::kSuccess, theme::kSecondary));
    }
    std::vector<std::string> right = boxLines(offers, 40, "Offers for you", theme::kAccent);
    return indentLines(sideBySide({left, right}, 2), 2);
}

std::string orderTable(const std::vector<OrderPtr>& orders, int selected, bool showCustomer) {
    std::vector<Column> cols = {{"Order", 6, Align::Left}, {"Date", 16, Align::Left}};
    if (showCustomer) cols.push_back({"Customer", 16, Align::Left});
    cols.push_back({"Items", 5, Align::Center});
    cols.push_back({"Total", 12, Align::Right});
    cols.push_back({"Paid via", 8, Align::Left});
    cols.push_back({"Status", 11, Align::Left});
    Table t(cols);
    for (const OrderPtr& o : orders) {
        std::vector<std::string> row = {style::paintBold("#" + toStr(o->getId()), theme::kSecondary),
                                        style::muted(o->getDate())};
        if (showCustomer) row.push_back(util::truncate(o->getCustomerName(), 16));
        row.push_back(toStr(o->units()));
        row.push_back(style::paintBold(money(o->total()), kWhite));
        row.push_back(shortPayment(o->getPaymentMethod()));
        row.push_back(statusBadge(o->getStatus()));
        t.addRow(row);
    }
    if (orders.empty()) {
        std::vector<std::string> row(cols.size(), "");
        row[1] = style::muted("No orders yet");
        t.addRow(row);
    }
    t.setSelected(selected);
    return t.render(2);
}

std::string orderDetail(const Order& order) {
    std::string out;
    std::vector<std::string> info = {
        labelValue(style::muted("Order"), style::paintBold("#" + toStr(order.getId()), theme::kSecondary), 40),
        labelValue(style::muted("Placed on"), order.getDate(), 40),
        labelValue(style::muted("Status"), statusBadge(order.getStatus()), 40),
        labelValue(style::muted("Payment"), shortPayment(order.getPaymentMethod()), 40),
        labelValue(style::muted("Reference"), order.getPaymentRef(), 40),
        labelValue(style::muted("Ship to"), util::truncate(order.getAddress(), 30), 40),
    };
    std::vector<std::string> left = boxLines(info, 40, "Order info", theme::kPrimary);
    std::vector<std::string> right = boxLines(breakdownLines(order.totals(), 40), 40, "Bill",
                                              theme::kSecondary);
    out += indentLines(sideBySide({left, right}, 2), 2);

    Table t({{"Product", 34, Align::Left},
             {"Category", 12, Align::Left},
             {"Unit", 11, Align::Right},
             {"Qty", 4, Align::Center},
             {"Total", 12, Align::Right}});
    for (const OrderLine& l : order.getLines()) {
        t.addRow({util::truncate(l.name, 34), categoryTag(l.category), money(l.unitFinal()),
                  toStr(l.quantity), style::paintBold(money(l.lineTotal()), kWhite)});
    }
    out += t.render(2);
    return out;
}

std::string profileCard(const Customer& c, int orderCount, const Money& spent) {
    std::string initials;
    std::string name = c.getName();
    initials += name.empty() ? '?' : name[0];
    std::size_t sp = name.rfind(' ');
    if (sp != std::string::npos && sp + 1 < name.size()) initials += name[sp + 1];
    const int w = 62;
    std::vector<std::string> lines = {
        style::badge(" " + util::toUpper(initials) + " ", theme::kPrimary, kWhite) + "  " +
            style::paintBold(c.getName(), kWhite) + "  " + style::muted(c.getRole()),
        "",
        style::muted(padRight("Email", 14)) + c.getEmail(),
        style::muted(padRight("Phone", 14)) + (c.getPhone().empty() ? style::muted("-") : c.getPhone()),
        style::muted(padRight("Address", 14)) + util::truncate(c.getAddress(), 46),
        style::muted(padRight("Member since", 14)) + c.getJoinedOn(),
        "",
        style::muted(padRight("Wallet", 14)) + style::paintBold(c.getWallet().str(), theme::kSuccess),
        style::muted(padRight("Orders", 14)) + style::paintBold(toStr(orderCount), theme::kSecondary) +
            style::muted("   total spent ") + style::paintBold(spent.str(), theme::kSecondary),
    };
    return box(lines, w, "My profile", theme::kPrimary, 2);
}

// ============================================================================
// Admin
// ============================================================================

std::string kpiTiles(const SalesReport& r, std::size_t productCount, std::size_t customerCount) {
    struct Tile {
        std::string label;
        std::string value;
        std::string caption;
        RGB color;
    };
    std::vector<Tile> tiles = {
        {"REVENUE", r.revenue.shortStr(), toStr(r.orders) + " paid orders", theme::kSuccess},
        {"AVG ORDER", r.averageOrder.shortStr(), "coupons saved " + r.couponSavings.shortStr(),
         theme::kSecondary},
        {"UNITS SOLD", toStr(r.units), toStr(r.cancelled) + " order(s) cancelled", theme::kAccent},
        {"CATALOG", toStr(productCount) + " items", toStr(customerCount) + " customers", theme::kGold},
    };
    std::vector<std::vector<std::string>> blocks;
    for (const Tile& t : tiles) {
        blocks.push_back(boxLines({style::muted(t.label), style::paintBold(t.value, t.color),
                                   style::paint(util::truncate(t.caption, 18), theme::kFaint)},
                                  18, "", t.color));
    }
    return indentLines(sideBySide(blocks, 2), 2);
}

std::vector<std::string> salesCharts(const SalesReport& r, double progress) {
    std::vector<Bar> cats;
    for (const std::string& name : {std::string("Electronics"), std::string("Clothing"),
                                    std::string("Grocery")}) {
        auto it = r.revenueByCategory.find(name);
        Money m = it == r.revenueByCategory.end() ? Money() : it->second;
        auto u = r.unitsByCategory.find(name);
        int units = u == r.unitsByCategory.end() ? 0 : u->second;
        cats.push_back({name, m.rupees(), m.shortStr() + style::muted("  " + toStr(units) + " units"),
                        theme::forCategory(name)});
    }
    std::vector<std::string> left = boxLines(barChart(cats, 12, 46, progress), 88,
                                             "Revenue by category (net item value)", theme::kSecondary);

    std::vector<Bar> top;
    const RGB palette[5] = {theme::kPrimary, theme::kSecondary, theme::kAccent, theme::kGold,
                            theme::kSuccess};
    for (std::size_t i = 0; i < r.topProducts.size(); ++i) {
        top.push_back({r.topProducts[i].first, static_cast<double>(r.topProducts[i].second),
                       toStr(r.topProducts[i].second) + " units", palette[i % 5]});
    }
    std::vector<std::string> topLines = barChart(top, 30, 40, progress);
    if (topLines.empty()) topLines.push_back(style::muted("No sales yet."));
    std::vector<std::string> mid = boxLines(topLines, 88, "Top products by units sold", theme::kAccent);

    std::vector<Bar> pay;
    std::size_t i = 0;
    for (const auto& kv : r.ordersByPayment) {
        pay.push_back({shortPayment(kv.first) + " (" + toStr(kv.second) + ")",
                       static_cast<double>(kv.second), "", palette[i++ % 5]});
    }
    std::vector<std::string> payLines = pay.empty() ? std::vector<std::string>{style::muted("-")}
                                                    : shareBar(pay, 88);
    std::vector<std::string> bottom = boxLines(payLines, 88, "Payment mix (orders)", theme::kPrimary);

    std::vector<std::string> out;
    out.insert(out.end(), left.begin(), left.end());
    out.insert(out.end(), mid.begin(), mid.end());
    out.insert(out.end(), bottom.begin(), bottom.end());
    return out;
}

void animatedSalesCharts(const SalesReport& r) {
    if (!Terminal::animationEnabled()) {
        Terminal::write(indentLines(salesCharts(r, 1.0), 2));
        return;
    }
    Terminal::hideCursor();
    const int frames = 24;
    std::size_t height = 0;
    for (int f = 1; f <= frames; ++f) {
        double t = static_cast<double>(f) / frames;
        double eased = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
        std::vector<std::string> lines = salesCharts(r, eased);
        if (f > 1) Terminal::cursorUp(static_cast<int>(height));
        height = lines.size();
        Terminal::write(indentLines(lines, 2));
        Terminal::flush();
        Terminal::sleepMs(30);
    }
    Terminal::showCursor();
}

std::string lowStockPanel(const std::vector<ProductPtr>& low) {
    std::vector<std::string> lines;
    for (const ProductPtr& p : low) {
        double fill = std::min(1.0, p->getStock() / 5.0);
        lines.push_back(style::paintBold("!", theme::kWarning) + " " +
                        padRight(util::truncate(p->getName(), 34), 34) + " " +
                        meter(fill, 12, theme::kDanger, theme::kWarning) + " " +
                        style::paintBold(p->getStock() == 0 ? "out of stock"
                                                            : toStr(p->getStock()) + " left",
                                         p->getStock() <= 2 ? theme::kDanger : theme::kWarning) +
                        style::muted("  #" + toStr(p->getId())));
    }
    if (lines.empty()) lines.push_back(style::paint("All products are well stocked.", theme::kSuccess));
    return box(lines, 88, "Low-stock alerts (5 or fewer)", theme::kWarning, 2);
}

}  // namespace screens
}  // namespace ui
}  // namespace shop
