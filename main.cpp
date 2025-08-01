#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <iostream>
#include <ranges>
#include "OrderBook.h"
#include "util/Logger.h"

extern int matches;

template <typename T>
T randval(T min, T max) {
    if constexpr (std::is_integral<T>::value) {
        return ((T)rand() % (max - min + 1)) + min;
    } else if constexpr (std::is_floating_point<T>::value) {
        return min + static_cast<T>(rand()) / (static_cast<T>(RAND_MAX) / (max - min));
    }
}

Side randomSide() {
    return randval<int>(0, 1) == 0 ? BUY : SELL;
}

void signal_handler(int signum) {
    if (signum == SIGINT) {
        // Handle the Ctrl+C interrupt
        // For example, clean up resources, save data, and exit gracefully
        // Logger::getInstance().info("Ctrl+C detected! Exiting gracefully...");
        // Logger::getInstance().info("Found " + std::to_string(matches) + " matches.");
        std::cout << "Ctrl+C detected! Exiting gracefully..." << std::endl;
        std::cout << "Found " << matches << " matches." << std::endl;
        exit(signum);  // Terminate the program with the signal code
    }
}

void addMarketOrders(OrderBook& ob,size_t num_orders) {
    for (size_t i = 0; i < num_orders; i++) {
        Order order(Order::createMarketOrder(
            randomSide(),
            randval<uint32_t>(100, 110)));
        ob.add_order(order);
    }
}

int main(int argc, char const *argv[]) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile("hft_sim.log");
    logger.setLogLevel(LogLevel::INFO);
    logger.enableConsole(false);
    logger.enableFile(true);
    logger.info("Simulation started");
    // Seed the random number generator
    srand(time(nullptr));
    OrderBook lob;  // Create limit order book

    // Create a dedicated thread to manage logging
    // std::thread logger_thread(&Logger::run, &logger);
    // logger_thread.detach();  // Detach the thread to run independently

    // View orders (Expectation is they should be sorted, per std::map implementation)
    auto bids = lob.getBids();
    auto asks = lob.getAsks();

    signal(SIGINT, signal_handler);
    // Simulate the market
    while(1) {
        Order order(
            randomSide(), // Side (BUY/SELL)
            (Type)(randval<int>(0, 1)), // Market/Limit orders
            randval<float>(50, 54), // Price [50, 54]
            randval<uint32_t>(100, 110)); // Order size
        lob.add_order(order);
        if (lob.getBids().size() > 2 and lob.getAsks().size() > 2) {
            // puts("Matching orders...");
            lob.match_orders();
        }
    }


    return 0;
}
