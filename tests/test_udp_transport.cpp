#include <gtest/gtest.h>

#include "net/UdpReliable.h"
#include "net/Transport.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(UdpReliable, LoopbackRoundtrip) {
    hsnet::UdpConfig cfgA;
    cfgA.localEndpoint = "127.0.0.1:9101";
    cfgA.remoteEndpoint = "127.0.0.1:9102";

    hsnet::UdpConfig cfgB;
    cfgB.localEndpoint = "127.0.0.1:9102";
    cfgB.remoteEndpoint = "127.0.0.1:9101";

    auto ta = hsnet::makeUdpReliableTransport(cfgA);
    auto tb = hsnet::makeUdpReliableTransport(cfgB);

    auto pubA = ta->createPublication(cfgA.remoteEndpoint, cfgA.streamId);
    auto subB = tb->createSubscription(cfgB.localEndpoint, cfgB.streamId);

    const char msg[] = "hello";
    auto res = pubA->offer(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(msg), sizeof(msg)-1), cfgA.streamId, true);
    ASSERT_EQ(res, hsnet::PublishResult::OK);

    std::atomic<bool> received{false};
    auto start = std::chrono::steady_clock::now();
    while (!received && std::chrono::steady_clock::now() - start < 1s) {
        subB->poll([&](const hsnet::MessageView& mv){
            ASSERT_EQ(mv.length, sizeof(msg)-1);
            std::string s(reinterpret_cast<const char*>(mv.data), mv.length);
            EXPECT_EQ(s, "hello");
            received = true;
        }, 8);
        std::this_thread::sleep_for(1ms);
    }
    EXPECT_TRUE(received);
} 