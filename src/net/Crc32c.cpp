#include "net/Crc32c.h"

#include <arm_acle.h>
#include <cstddef>
#include <cstdint>

namespace hsnet {

uint32_t crc32c_hw(const uint8_t* data, size_t len) noexcept {
    // Stub: return 0 for now; replace with SSE4.2/ARMv8 accelerated implementation
    (void)data; (void)len; return 0u;
}

uint32_t crc32c_sw(const uint8_t* data, size_t len) noexcept {
    // Stub: simple XOR placeholder
    uint32_t c = 0u;
    for (size_t i = 0; i < len; ++i) c ^= data[i];
    return c;
}

uint32_t crc32c(const uint8_t* data, size_t len) noexcept {
    return crc32c_sw(data, len);
}

} // namespace hsnet 