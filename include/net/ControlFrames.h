#pragma once

#include <cstdint>

namespace hsnet::ctrl {

struct Ack {
    uint64_t cumulative_ack;
    uint64_t sack_bitmap;
};

struct NakRange {
    uint64_t start_seq;
    uint64_t end_seq;
};

struct Nak {
    uint16_t count;
};

} // namespace hsnet::ctrl 