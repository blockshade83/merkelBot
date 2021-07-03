#include <iostream>
#include <string>
#include <vector>
#include "OrderBookEntry.h"
#include "MerkelBot.h"
#include "CSVReader.h"
#include <chrono>

int main() {

    // starting high resolution clock to measure program running time
    auto start = std::chrono::high_resolution_clock::now();

    MerkelBot app{};
    app.init();

    // stopping high resolution clock to measure program running time
    auto stop = std::chrono::high_resolution_clock::now();
    // extracting duration and logging to console
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Program executed successfully in " << duration.count() << " microseconds" << std::endl;

    return 0;
}