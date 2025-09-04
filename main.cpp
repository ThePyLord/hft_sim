#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <ranges>
#include <thread>

#include "LockFreeQueue.h"
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

void addMarketOrders(OrderBook& ob, size_t num_orders) {
    for (size_t i = 0; i < num_orders; i++) {
        Order order(Order::createMarketOrder(
            randomSide(),
            randval<uint32_t>(100, 110)));
        ob.add_order(order);
    }
}

int main(int argc, char const* argv[]) {
    Logger& logger = Logger::getInstance();
    logger.setLogFile("hft_sim.log");
    logger.setLogLevel(LogLevel::INFO);
    logger.enableConsole(false);
    logger.enableFile(true);
    logger.info("Simulation started");
    // Seed the random number generator
    srand(time(nullptr));
    OrderBook lob;  // Create limit order book
    LockFreeQueue<Order> order_queue;
    // Create a dedicated thread to manage logging
    // std::thread logger_thread(&Logger::run, &logger);
    // logger_thread.detach();  // Detach the thread to run independently

    // View orders (Expectation is they should be sorted, per std::map implementation)
    auto bids = lob.getBids();
    auto asks = lob.getAsks();

    signal(SIGINT, signal_handler);
    // Simulate the market
    const size_t orders_per_producer = 10;
    const size_t num_producers = 2;
    const size_t num_consumers = 2;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    producers.reserve(num_producers);
    consumers.reserve(num_consumers);

    for (size_t i = 0; i < num_producers; ++i) {
        producers.emplace_back(
            [](LockFreeQueue<Order>& p_queue, uint32_t num_orders) {
                for (size_t j = 0; j < num_orders; ++j) {
                    Order order(
                        randomSide(),                  // Side (BUY/SELL)
                        (Type)(randval<int>(0, 1)),    // Market/Limit orders
                        randval<float>(50, 54),        // Price [50, 54]
                        randval<uint32_t>(100, 110));  // Order size
                    p_queue.push_back(order);
                }
            },
            std::ref(order_queue),
            orders_per_producer);
    }

    for (size_t i = 0; i < num_consumers; ++i) {
        consumers.emplace_back(
            [](LockFreeQueue<Order>& o_queue, OrderBook& lob) {
                while (true) {
                    std::optional<Order> order = o_queue.pop();
                    if (order.has_value()) {
                        lob.add_order(*order);
                    } else {
                        std::this_thread::yield();  // Yield if no orders are available
                    }
                }
            },
            std::ref(order_queue),
            std::ref(lob));
    }

    // while (1) {
    //     Order order(
    //         randomSide(),                  // Side (BUY/SELL)
    //         (Type)(randval<int>(0, 1)),    // Market/Limit orders
    //         randval<float>(50, 54),        // Price [50, 54]
    //         randval<uint32_t>(100, 110));  // Order size
    //     lob.add_order(order);
    //     if (lob.getBids().size() > 2 and lob.getAsks().size() > 2) {
    //         // puts("Matching orders...");
    //         lob.match_orders();
    //     }
    // }

    return 0;
}
