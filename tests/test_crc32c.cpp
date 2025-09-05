#include <gtest/gtest.h>
#include "net/Crc32c.h"

TEST(Crc32c, KnownVectors) {
    // Test vectors from RFC 3720 (iSCSI)
    const char test1[] = "";
    EXPECT_EQ(hsnet::crc32c(reinterpret_cast<const uint8_t*>(test1), 0), 0x00000000);
    
    const char test2[] = "a";
    EXPECT_EQ(hsnet::crc32c(reinterpret_cast<const uint8_t*>(test2), 1), 0xC1D04330);
    
    const char test3[] = "abc";
    EXPECT_EQ(hsnet::crc32c(reinterpret_cast<const uint8_t*>(test3), 3), 0x364B3FB7);
    
    const char test4[] = "message digest";
    EXPECT_EQ(hsnet::crc32c(reinterpret_cast<const uint8_t*>(test4), 14), 0x02BD79D0);
    
    const char test5[] = "abcdefghijklmnopqrstuvwxyz";
    EXPECT_EQ(hsnet::crc32c(reinterpret_cast<const uint8_t*>(test5), 26), 0x9EE6EF25);
}

TEST(Crc32c, SoftwareVsHardware) {
    const char test_data[] = "BUY AAPL 100 @ 150.00";
    size_t len = strlen(test_data);
    
    uint32_t sw_result = hsnet::crc32c_sw(reinterpret_cast<const uint8_t*>(test_data), len);
    uint32_t hw_result = hsnet::crc32c_hw(reinterpret_cast<const uint8_t*>(test_data), len);
    
    // Both should produce the same result
    EXPECT_EQ(sw_result, hw_result);
    
    // Main function should also produce the same result
    uint32_t main_result = hsnet::crc32c(reinterpret_cast<const uint8_t*>(test_data), len);
    EXPECT_EQ(main_result, sw_result);
}

TEST(Crc32c, LargeData) {
    // Test with larger data to ensure no buffer overflows
    std::vector<uint8_t> large_data(1000);
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    uint32_t result = hsnet::crc32c(large_data.data(), large_data.size());
    // Just ensure it doesn't crash and produces a non-zero result
    EXPECT_NE(result, 0x00000000);
} 