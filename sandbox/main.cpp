#include "Engine.h"
#include <chrono>
#include <iomanip>
#include <iostream>

int main() {
    std::cout << "Starting Columnar Engine..." << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();

    Engine eng("test.hub");
    eng.TakeAll("test.csv");
    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    std::cout << "\n=== Performance Results ===" << std::endl;
    std::cout << "Total execution time: " << std::fixed << std::setprecision(6) << duration.count() / 1000000.0
              << " seconds" << std::endl;
    std::cout << "Total execution time: " << duration.count() / 1000.0 << " milliseconds" << std::endl;
    std::cout << "Total execution time: " << duration.count() << " microseconds" << std::endl;

    return 0;
}