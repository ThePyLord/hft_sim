#include "net/Protocol.h"

#include <arpa/inet.h>
#include <cstring>

namespace hsnet::proto {

static constexpr uint32_t MAGIC = 0x57554E4E41u; // "WUNNA"
// static constexpr uint32_t MAGIC = 0x48535544u; // "HSUD"
static constexpr uint8_t VERSION = 1u;

void write_header(uint8_t* dst, const Header& h) noexcept {
    Header net_h = h;

    // Pack: magic (40 bits) | version (8 bits) | frame_type (8 bits) | flags (8 bits)
    uint64_t magic_version_type_flags =
        (static_cast<uint64_t>(MAGIC) << 24) |
        (static_cast<uint64_t>(VERSION) << 16) |
        (static_cast<uint64_t>(static_cast<uint8_t>(FrameType::DATA)) << 8) |
        (static_cast<uint64_t>(0) << 0); // flags
    net_h.magic_version_type_flags = magic_version_type_flags;

    // Convert to network byte order
    net_h.magic_version_type_flags = htonll(net_h.magic_version_type_flags);
    net_h.sequence_number = htonll(net_h.sequence_number);
    net_h.send_time_ns = htonll(net_h.send_time_ns);
    net_h.stream_id = htonl(net_h.stream_id);
    net_h.fragment_index = htons(net_h.fragment_index);
    net_h.fragments_total = htons(net_h.fragments_total);
    net_h.payload_length = htons(net_h.payload_length);
    net_h.reserved = htons(net_h.reserved);
    net_h.crc32c_payload = htonl(net_h.crc32c_payload);
    
    std::memcpy(dst, &net_h, sizeof(Header));
}

bool parse_header(const uint8_t* src, Header& out) noexcept {
    std::memcpy(&out, src, sizeof(Header));

    out.sequence_number = ntohll(out.sequence_number);
    out.magic_version_type_flags = ntohll(out.magic_version_type_flags);
    out.send_time_ns = ntohll(out.send_time_ns);
    out.stream_id = ntohl(out.stream_id);
    out.fragment_index = ntohs(out.fragment_index);
    out.fragments_total = ntohs(out.fragments_total);
    out.payload_length = ntohs(out.payload_length);
    out.reserved = ntohs(out.reserved);
    out.crc32c_payload = ntohl(out.crc32c_payload);
    // Basic validation
    if ((out.magic_version_type_flags >> 24) != MAGIC) {
        return false;
    }
    if (((out.magic_version_type_flags >> 16) & 0xFF) != VERSION) {
        return false;
    }
    return true;
}

} // namespace hsnet::proto 