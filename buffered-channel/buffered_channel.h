#pragma once

#include <utility>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stdexcept>

template <class T>
class BufferedChannel {
public:
    explicit BufferedChannel(int size) : max_size_(size) {
    }

    void Send(const T& value) {
        std::unique_lock<std::mutex> lock(global_);

        if (is_close_) {
            throw std::runtime_error("the channel is closed");
        }

        read_.wait(lock, [this]() { return cur_size_ <= max_size_ || is_close_; });

        if (is_close_) {
            throw std::runtime_error("the channel is closed");
        }

        ++cur_size_;
        data_.push(std::move(value));
        write_.notify_one();
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> lock(global_);
        write_.wait(lock, [this]() { return cur_size_ || is_close_; });

        if (cur_size_ == 0 && is_close_) {
            return std::nullopt;
        }

        auto cur_elem = data_.front();
        data_.pop();
        --cur_size_;
        read_.notify_all();
        return cur_elem;
    }

    void Close() {
        std::unique_lock<std::mutex> lock(global_);
        is_close_ = true;
        write_.notify_all();
    }

private:
    std::mutex global_;
    std::queue<T> data_;
    std::condition_variable read_;
    std::condition_variable write_;
    bool is_close_ = false;
    int cur_size_ = 0;
    int max_size_ = 0;
};
