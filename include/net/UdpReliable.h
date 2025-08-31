#pragma once

#include <memory>
#include <string>

#include "Transport.h"

namespace hsnet {

struct UdpConfig {
    std::string local_endpoint;   // "ip:port"
    std::string remote_endpoint;  // "ip:port"
    bool enable_crc32_c{false};
    uint32_t stream_id{1};
    uint32_t mtu{1500};
    uint32_t recvRingSize{1u << 16};
    uint32_t txRingSize{1u << 16};
    uint32_t retransmitRingSize{1u << 15};
};

struct FeedPublisherConfig {
    std::string local_interface; // "ip" or empty for default
    std::string multicast_group; // "ip"
    uint16_t port{8170};
    bool enable_crc32_c{false};
    uint32_t stream_id{1};
    uint32_t mtu{1500};
    uint32_t txRingSize{1u << 16};
};

struct FeedSubscriberConfig {
    std::string local_interface; // "ip" or empty for default
    std::string multicast_group; // "ip"
    uint16_t port{8170};
    bool enable_crc32_c{false};
    uint32_t stream_id{1};
    uint32_t mtu{1500};
    uint32_t recvRingSize{1u << 16};
};

std::unique_ptr<ITransport> make_udp_reliable_transport(const UdpConfig& cfg);

std::unique_ptr<IPublication> make_udp_reliable_publisher(const FeedPublisherConfig& pub_cfg);
std::unique_ptr<ISubscription> make_udp_reliable_subscriber(const FeedSubscriberConfig& sub_cfg);
} // namespace hsnet