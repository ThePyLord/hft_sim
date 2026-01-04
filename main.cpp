#include <algorithm>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <ranges>
#include <thread>

#include "LockFreeQueue.h"
#include "OrderBook.h"
#include "util/Logger.h"

extern int matches;
std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
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
		std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
		std::cout << "Total runtime: " << duration << " seconds." << std::endl;
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
	logger.setLogFile("rnd.log");
	logger.setLogLevel(LogLevel::INFO);
	logger.enableConsole(false);
	logger.enableFile(true);
	logger.info("Simulation started");
	// Seed the random number generator

    srand(time(nullptr));
	std::random_device rd;
    std::mt19937 gen(rd());

	std::uniform_int_distribution<> dis(0, 1);
    
	OrderBook lob;  // Create limit order book
	LockFreeQueue<Order> order_queue;

	signal(SIGINT, signal_handler);
	// Simulate the market
	const size_t orders_per_producer = 100;
	const size_t num_producers = 4;

	std::vector<std::thread> producers;
	std::atomic<bool> done{false};
	producers.reserve(num_producers);

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


	for (auto& producer : producers) {
		if (producer.joinable()) {
			producer.join();
		}
	}

	done.store(true, std::memory_order_release);

    std::thread consumer_thread([&]() {
	  while (true) {
            std::optional<Order> order = order_queue.pop();
            if (order.has_value()) {
              lob.add_order(*order);
            } else {
              if (done.load(std::memory_order_acquire)) {
                break;
              }
              std::this_thread::yield();
            }
	  }
        });
    consumer_thread.join();
    
	return 0;
}
