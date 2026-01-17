#include "core/Application.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    std::cout << "\n==================================" << std::endl;
    std::cout << "LIBRE DCC TOOL" << std::endl;
    std::cout << "Starting..." << std::endl;
    std::cout << "==================================\n" << std::endl;

    Application app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "\n[SUCCESS] Program completed!" << std::endl;
    return EXIT_SUCCESS;
}