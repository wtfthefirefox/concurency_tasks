#pragma once

#include <atomic>
#include <optional>
#include <stdexcept>
#include <utility>

template <class T>
class MPSCStack {
    struct Node {
        T value_;
        Node* next_;

        Node(T val, Node* next) : value_(val), next_(next){};
    };

public:
    // Push adds one element to stack top.
    //
    // Safe to call from multiple threads.
    void Push(const T& value) {
        Node* new_node = new Node(value, head_);
        while (!head_.compare_exchange_weak(new_node->next_, new_node)) {
        }
    }

    // Pop removes top element from the stack.
    //
    // Not safe to call concurrently.
    std::optional<T> Pop() {
        if (head_ == nullptr) {
            return std::nullopt;
        }

        Node* head_node = head_.load();
        while (!head_.compare_exchange_weak(head_node, head_node->next_)) {
        }

        T val = std::move(head_node->value_);
        delete head_node;

        return val;
    }

    // DequeuedAll Pop's all elements from the stack and calls cb() for each.
    //
    // Not safe to call concurrently with Pop()
    template <class TFn>
    void DequeueAll(const TFn& cb) {
        std::optional<T> cur_val;

        while (head_) {
            cur_val = Pop();
            cb(cur_val.value());
        }
    }

    ~MPSCStack() {
        DequeueAll([](T) {});
    }

private:
    std::atomic<Node*> head_ = nullptr;
};
