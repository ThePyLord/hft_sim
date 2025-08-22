#pragma once

#include <cstdint>

namespace hsnet::proto {

enum class FrameType : uint8_t { 
	DATA=0, 
	ACK=1, 
	NAK=2, 
	HEARTBEAT=3, 
	HELLO=4, 
	RESET=5 
};

struct alignas(8) Header {
    uint64_t magic_version_type_flags;
    uint64_t sequence_number;
    uint64_t send_time_ns;
    uint32_t stream_id;
    uint16_t fragment_index;
    uint16_t fragments_total;
    uint16_t payload_length;
    uint16_t reserved;
    uint32_t crc32c_payload;
};

void writeHeader(uint8_t* dst, const Header& h) noexcept;
bool parseHeader(const uint8_t* src, Header& out) noexcept;

} // namespace hsnet::proto 