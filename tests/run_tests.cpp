#include "test_framework.hpp"
#include <iostream>
#include <chrono>

int main() {
    const auto& tests = test_framework::TestRegistry::instance().tests();
    std::cout << "=========================================================\n";
    std::cout << "Running " << tests.size() << " IndexForge Unit & Integration Tests\n";
    std::cout << "=========================================================\n\n";

    size_t passed = 0;
    size_t failed = 0;

    auto total_start = std::chrono::high_resolution_clock::now();

    for (const auto& test : tests) {
        std::cout << "[ RUN      ] " << test.name << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        try {
            test.func();
            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << "[       OK ] " << test.name << " (" << elapsed << " ms)\n";
            passed++;
        } catch (const std::exception& e) {
            std::cout << "[  FAILED  ] " << test.name << "\n";
            std::cout << "             " << e.what() << "\n";
            failed++;
        }
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    double total_elapsed = std::chrono::duration<double, std::milli>(total_end - total_start).count();

    std::cout << "\n=========================================================\n";
    std::cout << "Test Summary: " << passed << " passed, " << failed << " failed in "
              << total_elapsed << " ms\n";
    std::cout << "=========================================================\n";

    return (failed == 0) ? 0 : 1;
}
