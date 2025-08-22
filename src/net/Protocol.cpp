#include "net/Protocol.h"

#include <arpa/inet.h>
#include <cstring>

namespace hsnet::proto {

static constexpr uint32_t MAGIC = 0x48535544u; // "HSUD"
static constexpr uint8_t VERSION = 1u;

void writeHeader(uint8_t* dst, const Header& h) noexcept {
    // Pack in network byte order; for now just memcpy as a stub.
    std::memcpy(dst, &h, sizeof(Header));
    (void)MAGIC; (void)VERSION;
}

bool parseHeader(const uint8_t* src, Header& out) noexcept {
    std::memcpy(&out, src, sizeof(Header));
    return true;
}

} // namespace hsnet::proto 