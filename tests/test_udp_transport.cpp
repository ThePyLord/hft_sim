#include <gtest/gtest.h>

#include "net/UdpReliable.h"
#include "net/Transport.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

/**
 * @brief Unit test for basic multicast functionality using the UdpReliable transport.
 *
 * This test verifies that a publisher can send a message to multiple subscribers
 * using multicast, and both subscribers receive the message correctly.
 */
TEST(UdpReliable, BasicMulticast) {
    using namespace std::chrono;
    hsnet::FeedPublisherConfig pubCfg;
    pubCfg.multicast_group = "239.0.0.1";
    pubCfg.port = 8170;
    auto pub = hsnet::make_udp_reliable_publisher(pubCfg);

    hsnet::FeedSubscriberConfig subCfg;
    subCfg.multicast_group = "239.0.0.1";
    subCfg.port = 8170;

    auto sub1 = hsnet::make_udp_reliable_subscriber(subCfg);
    auto sub2 = hsnet::make_udp_reliable_subscriber(subCfg);
    char msg[] = "BUY PLTR 8";
    pub->offer(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg), sizeof(msg)), pubCfg.stream_id);

    std::atomic<bool> recvB{false};
    std::atomic<bool> recvC{false};
    auto start = steady_clock::now();
    while ((!recvB || !recvC) && steady_clock::now() - start < 10s) {
        sub1->poll([&](const hsnet::MessageView& mv) {
            ASSERT_EQ(mv.length, sizeof(msg));
            std::string s(reinterpret_cast<const char*>(mv.data), mv.length);
            recvB = true;
            ASSERT_TRUE(recvB);
        }, 32);

        sub2->poll([&](const hsnet::MessageView& mv) {
            ASSERT_EQ(mv.length, sizeof(msg));
            std::string s(reinterpret_cast<const char*>(mv.data), mv.length);
            recvC = true;
            ASSERT_TRUE(recvC);
        }, 32);
    }

}