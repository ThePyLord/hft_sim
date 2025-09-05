#include <algorithm>
#include <cassert>
#include "net/ReorderingBuffer.h"

namespace hsnet {

ReorderingBuffer::ReorderingBuffer(size_t max_size)
    : max_size_(max_size), next_seq_(0), head_(0), count_(0) {
    buffer_.resize(max_size);
}

bool ReorderingBuffer::add(uint64_t sequence, std::vector<uint8_t> data, uint32_t stream_id) {
    // If sequence is too old, ignore it
    if (sequence < next_seq_) {
        return false;
    }
    
    // If sequence is too far ahead, we might need to expand buffer
    if (sequence >= next_seq_ + max_size_) {
        // For now, just drop packets that are too far ahead
        // In a real implementation, we might want to expand or handle this differently
        return false;
    }
    
    // Calculate position in circular buffer
    size_t pos = (head_ + (sequence - next_seq_)) % max_size_;
    
    // If this position is already occupied, we have a duplicate
    if (buffer_[pos].valid && buffer_[pos].sequence == sequence) {
        return false;
    }
    
    // Store the packet
    if (!buffer_[pos].valid) {
        count_++;
    }
    buffer_[pos] = Packet(sequence, std::move(data), stream_id);

    return true;
}

std::optional<std::pair<std::vector<uint8_t>, uint32_t>> ReorderingBuffer::get_next() {
    if (!has_ready()) {
        return std::nullopt;
    }
    
    // Get the packet at head
    auto& packet = buffer_[head_];
    // Ensure it is valid and matches the expected sequence number
    assert(packet.valid && packet.sequence == next_seq_);
    
    // Extract data and stream_id
    auto data = std::move(packet.data);
    auto stream_id = packet.stream_id;
    
    // Mark slot as invalid
    packet.valid = false;
    packet.sequence = 0;
    packet.stream_id = 0;
    
    // Advance sequence number and head pointer
    next_seq_++;
    advance_head();
    count_--;
    
    return std::make_pair(std::move(data), stream_id);
}

bool ReorderingBuffer::has_ready() const {
    return buffer_[head_].valid && buffer_[head_].sequence == next_seq_;
}

void ReorderingBuffer::clear() {
    for (auto& packet : buffer_) {
        packet.valid = false;
        packet.sequence = 0;
        packet.data.clear();
    }
    next_seq_ = 0;
    head_ = 0;
    count_ = 0;
}

size_t ReorderingBuffer::find_packet(uint64_t sequence) const {
    if (sequence < next_seq_ || sequence >= next_seq_ + max_size_) {
        return max_size_; // Invalid position
    }
    
    size_t pos = (head_ + (sequence - next_seq_)) % max_size_;
    if (buffer_[pos].valid && buffer_[pos].sequence == sequence) {
        return pos;
    }
    
    return max_size_; // Not found
}

void ReorderingBuffer::advance_head() {
    // Move head to next valid packet or wrap around (MAINTAIN BUFFER INVARIANT)
    head_ = (head_ + 1) % max_size_;
}

} // namespace hsnet 