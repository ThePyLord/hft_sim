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

std::unique_ptr<ITransport> make_udp_reliable_transport(const UdpConfig& cfg);

} // namespace hsnet 