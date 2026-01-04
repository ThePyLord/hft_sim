#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <memory>

template <typename T>
class MemoryPool {
public:
	explicit MemoryPool(size_t pool_size = 1024): pool_size_(pool_size) {
		pool_ = std::make_unique<Slot[]>(pool_size_);
		init_free_list();
	};

	~MemoryPool() {};

	template<typename... Args>
	T* allocate(Args&&... args) {
		if(!head) 
			return nullptr;
		Slot* curr = head;
		head = head->next;
		return new (&curr->data) T(std::forward<Args>(args)...); // Pool exhausted
	};

	/**
	 * Marks a pooled object slot as free so it can be reused.
	 * @param obj Pointer previously obtained from this pool's allocate(). Must point
	 *            into the memory block returned by pool_.get().
	 * See also: allocate()
	 */
	void deallocate(T* obj) {
		
		if(!obj)
			return;
		obj->~T();

		Slot* curr = reinterpret_cast<Slot*>(obj);
		// Set the current slot to the top of the free list
		curr->next = head;
		head = curr;
	};

private:
	// Storage
	union Slot {
		T data;
		Slot* next;
	};
	std::unique_ptr<Slot[]> pool_;
	// free list
	Slot* head;
	// In-use buffer
	bool* in_use_;
	size_t pool_size_;

	void init_free_list() {
		std::cout << "Initializing free list..." << std::endl;
		for (size_t i = 0; i < pool_size_ - 1; ++i) {
			pool_[i].next = &pool_[i + 1];
		}
		pool_[pool_size_ - 1].next = nullptr;
		head = &pool_[0];
	}
};


#endif