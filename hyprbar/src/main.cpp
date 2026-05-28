#include "Bar.hpp"
#include <iostream>
#include <csignal>

static Bar* g_bar = nullptr;

static void sighandler(int) {
    // The bar's run() loop checks m_running via the display disconnect
    if (g_bar) {
        // Force the display to close
    }
    exit(0);
}

int main() {
    signal(SIGTERM, sighandler);
    signal(SIGINT,  sighandler);

    try {
        Bar bar;
        g_bar = &bar;
        bar.run();
    } catch (const std::exception& e) {
        std::cerr << "[hyprbar] Fatal: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
