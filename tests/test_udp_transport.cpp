#include <gtest/gtest.h>

#include "net/UdpReliable.h"
#include "net/Transport.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

TEST(UdpReliable, LoopbackRoundtrip) {
    hsnet::UdpConfig cfgA;
    cfgA.local_endpoint = "127.0.0.1:9101";
    cfgA.remote_endpoint = "127.0.0.1:9102";

    hsnet::UdpConfig cfgB;
    cfgB.local_endpoint = "127.0.0.1:9102";
    cfgB.remote_endpoint = "127.0.0.1:9101";

    // Transport A sends to B, Transport B receives from A
    auto ta = hsnet::make_udp_reliable_transport(cfgA);
    auto tb = hsnet::make_udp_reliable_transport(cfgB);

    auto pubA = ta->create_publication(cfgA.remote_endpoint, cfgA.stream_id);
    auto subB = tb->create_subscription(cfgB.local_endpoint, cfgB.stream_id);

    const char msg[] = "BUY AAPL 100";
    auto res = pubA->offer(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(msg), sizeof(msg)-1), cfgA.stream_id, true);
    ASSERT_EQ(res, hsnet::PublishResult::OK);

    std::atomic<bool> received{false};
    auto start = std::chrono::steady_clock::now();
    while (!received && std::chrono::steady_clock::now() - start < 1s) {
        subB->poll([&](const hsnet::MessageView& mv){
            ASSERT_EQ(mv.length, sizeof(msg)-1);
            std::string s(reinterpret_cast<const char*>(mv.data), mv.length);
            EXPECT_EQ(s, "BUY AAPL 100");
            received = true;
        }, 8);
        std::this_thread::sleep_for(1ms);
    }
    EXPECT_TRUE(received);
} 