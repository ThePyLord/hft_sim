#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace hsnet {

// Simple reordering buffer that ensures in-sequence delivery
class ReorderingBuffer {
public:
    explicit ReorderingBuffer(size_t max_size = 1024);
    
    // Add a packet with sequence number, returns true if it was added
    bool add(uint64_t sequence, std::vector<uint8_t> data, uint32_t stream_id = 0);
    
    // Get next in-sequence packet, returns empty if none available
    std::optional<std::pair<std::vector<uint8_t>, uint32_t>> get_next();
    
    // Check if we have any packets ready for delivery
    bool has_ready() const;
    
    // Get the next expected sequence number
    uint64_t next_expected() const { return next_seq_; }
    
    // Get statistics
    size_t size() const { return count_; }
    size_t max_size() const { return max_size_; }
    
    // Clear the buffer (useful for reset scenarios)
    void clear();

private:
    struct Packet {
        uint64_t sequence;
        std::vector<uint8_t> data;
        uint32_t stream_id;
        bool valid;
        
        Packet() : sequence(0), stream_id(0), valid(false) {}
        Packet(uint64_t seq, std::vector<uint8_t> d, uint32_t sid) : sequence(seq), data(std::move(d)), stream_id(sid), valid(true) {}
    };
    
    std::vector<Packet> buffer_;
    size_t max_size_;
    uint64_t next_seq_;
    size_t head_;
    size_t count_;
    
    // Helper to find packet in buffer
    size_t find_packet(uint64_t sequence) const;
    
    // Helper to advance head pointer
    void advance_head();
};

} // namespace hsnet 