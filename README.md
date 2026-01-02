# HFT Simulator (High-Frequency Trading Simulator)

This project seeks to explore the world of high-frequency trading (HFT) by simulating a trading environment. 
The simulator allows users to test various trading strategies, analyze market data, and understand the dynamics of HFT.

## Features

- Simulated market environment
- Order book management
  - Order cancellation
- Feed publishing and subscription
- Trade execution simulation
- Lock-free multi-producer single-consumer (MPSC) queue for high-performance message passing
- Basic logging functionality


## Things to work on / TODOs
- Performance optimizations (order matching and execution)
- Price representation as floats (should be a "Money" type)
- More sophisticated order types (e.g., stop-loss, iceberg orders)
- Enhanced logging and monitoring capabilities

# Potential future features
- Integration with real market data feeds
- Support for additional asset classes (e.g., options, futures)
- Advanced analytics and reporting tools
- 

# Getting Started

## Prerequisites
- C++17 or later
- CMake 3.20 or later
- A C++ compiler that supports C++17 (e.g., GCC, Clang, MSVC)
## Testing
To run all tests, use the following command after building the project:

```bash
cd build/
ctest 
```


## References
While I was working on the lock-free MPSC queue, I found the following resources helpful:
 - [Lock-Free Queue Implementation](https://people.cs.pitt.edu/~jacklange/teaching/cs2510-f17/implementing_lock_free.pdf)
 - [Lock-Free MPMC](https://www.linuxjournal.com/content/lock-free-multi-producer-multi-consumer-queue-ring-buffer)

