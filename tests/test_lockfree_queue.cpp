#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iostream>
#include "LockFreeQueue.h"

// Suite: LockFreeQueueTest_Basic
// Purpose: Test basic push and pop operations for correctness.
TEST(LockFreeQueueTest_Basic, PushPopSingle) {
    LockFreeQueue<int> q;
    int value = 42;
    q.push_back(value);

    int result = 42;
    // This will fail (TDD) until pop is implemented.
    auto out = q.pop();
    EXPECT_TRUE(out);
    EXPECT_EQ(result, value);
}

TEST(LockFreeQueueTest_Basic, PushMultiplePopOrder) {
    LockFreeQueue<int> q;
    for (int i = 0; i < 5; ++i) q.push_back(i);

    for (int i = 0; i < 5; ++i) {
        auto result = q.pop();
        EXPECT_TRUE(result);
        EXPECT_EQ(result, i);
    }
}

// Suite: LockFreeQueueTest_Empty
// Purpose: Ensure popping from an empty queue behaves correctly.
TEST(LockFreeQueueTest_Empty, PopEmptyReturnsFalse) {
    LockFreeQueue<int> q;
    auto result = q.pop();
    EXPECT_FALSE(result.has_value());
}

// Suite: LockFreeQueueTest_Concurrency
// Purpose: Test thread safety under concurrent push and pop.
// Tests SP-SC
TEST(LockFreeQueueTest_Concurrency, ConcurrentPushPop) {
    LockFreeQueue<int> q;
    std::atomic<int> pop_count{0};
    const int N = 10;

    auto producer = [&q, N]() {
        for (int i = 0; i < N; ++i) q.push_back(i);
    };

    auto consumer = [&q, &pop_count, N]() {
        // int val;
        for (int i = 0; i < N; ++i) {
            auto val = q.pop();
            while (!val.has_value()) std::this_thread::yield();
            // while (val) std::this_thread::yield();
            ++pop_count;
        }
    };

     std::thread t1(producer), t2(consumer);
     t1.join();
     t2.join();

    EXPECT_EQ(pop_count, N);
}

// Benchmark: Single-threaded push and pop
TEST(LockFreeQueueBenchmark, SingleThreaded) {
    LockFreeQueue<int> q;
    const int N = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N; ++i) q.push_back(i);
    // int val;
    for (int i = 0; i < N; ++i) q.pop();
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "[SingleThreaded] " << N << " push+pop in " << ms << " ms, "
              << (N * 2 / ms * 1000) << " ops/sec" << std::endl;
}

// Benchmark: Producer-consumer (2 threads)
TEST(LockFreeQueueBenchmark, ProducerConsumer) {
    LockFreeQueue<int> q;
    const int N = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    std::thread producer([&q, N]() {
        for (int i = 0; i < N; ++i) q.push_back(i);
    });
    std::thread consumer([&q, N]() {
        for (int i = 0; i < N; ++i) {
            auto val = q.pop();
            while (!val) std::this_thread::yield();
        }
    });
    producer.join();
    consumer.join();
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "[ProducerConsumer] " << N << " push+pop in " << ms << " ms, "
              << (N * 2 / ms * 1000) << " ops/sec" << std::endl;
}
