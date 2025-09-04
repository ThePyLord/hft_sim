#include <gtest/gtest.h>
#include "net/ReorderingBuffer.h"

#include <string>
#include <vector>

TEST(ReorderingBuffer, BasicOperations) {
    hsnet::ReorderingBuffer buffer(4);
    
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.next_expected(), 0);
    EXPECT_FALSE(buffer.has_ready());
}

TEST(ReorderingBuffer, InSequenceDelivery) {
    hsnet::ReorderingBuffer buffer(4);
    
    // Add packets in sequence
    std::vector<uint8_t> data1 = {1, 2, 3};
    std::vector<uint8_t> data2 = {4, 5, 6};
    
    EXPECT_TRUE(buffer.add(0, data1, 0));
    EXPECT_TRUE(buffer.add(1, data2, 1));
    
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_TRUE(buffer.has_ready());
    
    // Get first packet
    auto result1 = buffer.get_next();
    EXPECT_TRUE(result1.has_value());
    auto d1 = *result1;
    EXPECT_EQ(d1.first, data1);
    EXPECT_EQ(buffer.next_expected(), 1);
    // Get second packet
    auto result2 = buffer.get_next();

    EXPECT_TRUE(result2.has_value());
    d1 = *result2;
    EXPECT_EQ(d1.first, data2);
    EXPECT_EQ(buffer.next_expected(), 2);
    
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_FALSE(buffer.has_ready());
}

TEST(ReorderingBuffer, OutOfOrderDelivery) {
    hsnet::ReorderingBuffer buffer(4);
    
    std::vector<uint8_t> data1 = {1, 2, 3};
    std::vector<uint8_t> data2 = {4, 5, 6};
    std::vector<uint8_t> data3 = {7, 8, 9};
    
    // Add packets out of order: 1, 3, 2
    EXPECT_TRUE(buffer.add(0, data1));
    EXPECT_TRUE(buffer.add(2, data3, 2));
    EXPECT_TRUE(buffer.add(1, data2, 3));
    
    EXPECT_EQ(buffer.size(), 3);
    EXPECT_TRUE(buffer.has_ready());
    
    // Should deliver in sequence: 0, 1, 2
    auto result1 = buffer.get_next();
    EXPECT_TRUE(result1.has_value());
    auto p = *result1;
    EXPECT_EQ(p.first, data1);
    
    auto result2 = buffer.get_next();
    EXPECT_TRUE(result2.has_value());
    p = *result2;
    EXPECT_EQ(p.first, data2);
    
    auto result3 = buffer.get_next();
    EXPECT_TRUE(result3.has_value());
    p = *result3;
    EXPECT_EQ(p.first, data3);
    
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_FALSE(buffer.has_ready());
}

TEST(ReorderingBuffer, DuplicatePackets) {
    hsnet::ReorderingBuffer buffer(4);
    
    std::vector<uint8_t> data = {1, 2, 3};
    
    // Add same packet twice
    EXPECT_TRUE(buffer.add(0, data));
    EXPECT_FALSE(buffer.add(0, data)); // Duplicate should be ignored
    
    EXPECT_EQ(buffer.size(), 1);
    EXPECT_TRUE(buffer.has_ready());
}

TEST(ReorderingBuffer, OldPackets) {
    hsnet::ReorderingBuffer buffer(4);
    
    std::vector<uint8_t> data = {1, 2, 3};
    
    // Advance sequence to 5
    buffer.add(5, data);
    buffer.get_next(); // Consume packet 5
    
    // Try to add old packet
    EXPECT_FALSE(buffer.add(4, data)); // Old packet should be ignored
    EXPECT_EQ(buffer.size(), 0);
}

TEST(ReorderingBuffer, Clear) {
    hsnet::ReorderingBuffer buffer(4);
    
    std::vector<uint8_t> data = {1, 2, 3};
    buffer.add(0, data);
    
    EXPECT_EQ(buffer.size(), 1);
    
    buffer.clear();
    
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.next_expected(), 0);
    EXPECT_FALSE(buffer.has_ready());
} 