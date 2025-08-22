#pragma once

#include <memory>
#include <string>

#include "Transport.h"

namespace hsnet {

struct UdpConfig {
    std::string localEndpoint;   // "ip:port"
    std::string remoteEndpoint;  // "ip:port"
    bool enableCrc32c{false};
    uint32_t streamId{1};
    uint32_t mtu{1500};
    uint32_t recvRingSize{1u << 16};
    uint32_t txRingSize{1u << 16};
    uint32_t retransmitRingSize{1u << 15};
};

std::unique_ptr<ITransport> makeUdpReliableTransport(const UdpConfig& cfg);

} // namespace hsnet 