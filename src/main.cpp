// ShopSphere - a C++ object-oriented console e-commerce store.
// OOP course project, Group 17, Dept. of Computer Engineering, VIT Pune.

#include <exception>
#include <iostream>
#include <string>

#include "services/Store.h"
#include "ui/App.h"
#include "ui/Demo.h"
#include "ui/Terminal.h"
#include "util/Exceptions.h"

namespace {

void printUsage() {
    std::cout << "ShopSphere - C++ console shopping system\n\n"
                 "Usage: shopsphere [options]\n\n"
                 "  --demo            run the scripted feature walkthrough and exit\n"
                 "  --no-anim         disable animations and delays\n"
                 "  --no-color        plain output without ANSI colours\n"
                 "  --data <dir>      folder with the CSV data files (default: data)\n"
                 "  --invoices <dir>  folder for generated invoices (default: invoices)\n"
                 "  --reset-data      rewrite the data folder with the built-in seed data\n"
                 "  --help            show this help\n\n"
                 "Demo logins: tanaya@shop.com / pass123   admin / admin123\n";
}

}  // namespace

int main(int argc, char** argv) {
    using namespace shop;

    bool demo = false;
    bool animations = true;
    bool colors = true;
    bool reset = false;
    std::string dataDir = "data";
    std::string invoiceDir = "invoices";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--demo") demo = true;
        else if (arg == "--no-anim" || arg == "--no-animation") animations = false;
        else if (arg == "--no-color" || arg == "--no-colour") colors = false;
        else if (arg == "--reset-data") reset = true;
        else if (arg == "--data" && i + 1 < argc) dataDir = argv[++i];
        else if (arg == "--invoices" && i + 1 < argc) invoiceDir = argv[++i];
        else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n\n";
            printUsage();
            return 2;
        }
    }

    ui::Terminal::init(colors, animations);
    int code = 0;
    try {
        Store store(dataDir, invoiceDir);
        if (reset) {
            store.loadSeed();
            store.save();
            std::cout << "Seed data written to " << dataDir << "/ ("
                      << store.products().size() << " products, " << store.customers().size()
                      << " customers, " << store.orders().size() << " orders).\n";
        } else if (demo) {
            store.loadSeed();             // fixed dataset, never read from or written to disk
            store.setPersistence(false);
            ui::Demo show(store);
            code = show.run();
        } else {
            store.load();
            ui::App app(store);
            code = app.run();
        }
    } catch (const ui::InputClosed&) {
        code = 0;
    } catch (const ShopException& e) {   // OOP: exception handling -- last line of defence
        std::cerr << "\n" << e.title() << ": " << e.what() << "\n";
        code = 1;
    } catch (const std::exception& e) {
        std::cerr << "\nUnexpected error: " << e.what() << "\n";
        code = 1;
    }
    ui::Terminal::restore();
    return code;
}
