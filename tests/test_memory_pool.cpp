#include <gtest/gtest.h>
#include "MemoryPool.h"

TEST(MemoryPoolTest, AcquireAndRelease) {
	const size_t POOL_SIZE = 10;
	MemoryPool<int> intPool(POOL_SIZE);

	// Allocate all objects
	int* nums[POOL_SIZE]; 
	for (size_t i = 0; i < POOL_SIZE; ++i) {
		nums[i] = intPool.allocate();
	}

	// // Pool should be exhausted now
	int* val = intPool.allocate();
	ASSERT_EQ(intPool.allocate(), nullptr);

	// // Release one object
	intPool.deallocate(nums[5]);

	// // Now we should be able to acquire one more object
	int* newObj = intPool.allocate();
	ASSERT_NE(newObj, nullptr);
	*newObj = 42;
	ASSERT_EQ(*newObj, 42);
}