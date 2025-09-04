#ifndef LOCKFREEQUEUE_H
#define LOCKFREEQUEUE_H

#include <atomic>
#include <optional>
#include <memory>
#include "Order.h"

template <typename T>
class LockFreeQueue {
   private:
    struct DummyTag {};  // Dummy tag for the dummy node
    struct Node {
        std::optional<T> data;  // Ideally for our Order (but made generic)
        std::atomic<Node*> next;
        Node() : data(std::nullopt), next(nullptr) {};
        explicit Node(const T& val) : data(val), next(nullptr) {}
        explicit Node(T&& val) : data(std::move(val)), next(nullptr) {}
        // Node(DummyTag t) : next(nullptr) {}
        ~Node() = default;
    };
    std::atomic<Node*> head;
    std::atomic<Node*> tail;

   public:
    LockFreeQueue() {
        Node* dummy = new Node();
        head.store(dummy);
        tail.store(dummy);
    }

    ~LockFreeQueue() {
        // while (pop()) {
        //     Node* dummy = head.load(std::memory_order_relaxed);
        //     delete dummy;
        // }
        while (auto old_head = head.load(std::memory_order_acquire)) {
           head.store(old_head->next);
           delete old_head;
        }
    }

    void push_back(const T& val) {
        Node* new_node = new Node(val);
        Node* curr;
        while (true) {
            // Load the current tail node
            curr = tail.load(std::memory_order_acquire);
            Node* next = curr->next.load(std::memory_order_acquire);
            // If the tail is still the same, try to link the new node
            if (curr == tail.load(std::memory_order_acquire)) {
                if (next == nullptr) {
                    if (curr->next.compare_exchange_weak(next, new_node)) {
                        break;
                    }
                } else {
                    // Tail is lagging behind, try to advance it
                    // a typical approach would look like this:
                    // tail = next;
                    tail.compare_exchange_weak(curr, next);
                }
            }
        }
        // Now we have successfully linked the new node
        tail.compare_exchange_weak(curr, new_node);
        // Update the tail pointer to point to the new node
        tail.store(new_node, std::memory_order_release);
        // std::cout << "Pushed: " << val << std::endl;
    }

    bool pop(T& item) {
        Node* old_head = nullptr;
        while (true) {
            old_head = head.load(std::memory_order_acquire);
            Node* next = old_head->next.load(std::memory_order_acquire);
            if (next == nullptr) return false;

            if (head.compare_exchange_weak(old_head, next)) {
                item = next->data;
                delete old_head;
                return true;
            }
        }
    }

    std::optional<T> pop() {
        while (true) {
            Node* curr = head.load(std::memory_order_acquire);
            Node* next = curr->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                return std::nullopt;  // Queue is empty
            }
            if (head.compare_exchange_weak(curr, next,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
                std::optional<T>result(std::move(next->data));
                delete curr;
                // if (next == nullptr) {
                //     tail.store(new Node(DummyTag{}), std::memory_order_release);
                // }
                // break;
                return result;
            }
        }
    }
};

#endif  // LOCKFREEQUEUE_H
