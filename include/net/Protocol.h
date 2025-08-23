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

// Protocol header
// Using 64-bit fields to prevent sequence number wraparound and to allow for future expansion
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

void write_header(uint8_t* dst, const Header& h) noexcept;
bool parse_header(const uint8_t* src, Header& out) noexcept;

// define htonll and ntohll if not available
#if !defined(htonll) && !defined(ntohll)
	inline uint64_t htonll(uint64_t value) noexcept {
	    if constexpr (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
	        return __builtin_bswap64(value);
	    } else {
	        return value;
	    }
	}
	inline uint64_t ntohll(uint64_t value) noexcept {
	    if constexpr (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
	        return __builtin_bswap64(value);
	    } else {
	        return value;
	    }
	}
#endif

} // namespace hsnet::proto 