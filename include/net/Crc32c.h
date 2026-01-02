#pragma once

#include <cstddef>
#include <cstdint>

namespace hsnet {

uint32_t crc32c_hw(const uint8_t* data, size_t len) noexcept;
uint32_t crc32c_sw(const uint8_t* data, size_t len) noexcept;
uint32_t crc32c(const uint8_t* data, size_t len) noexcept;

} // namespace hsnet 