#pragma once

#include <utility>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <future>

template <class T>
class UnbufferedChannel {
public:
    void Send(const T& value) {
        std::unique_lock<std::mutex> lock(global_);

        if (is_close_) {
            throw std::runtime_error("the channel is closed");
        }

        read_.wait(lock, [this]() { return cur_size_ < max_size_ || is_close_; });

        if (is_close_) {
            throw std::runtime_error("the channel is closed");
        }

        ++cur_size_;
        write_.notify_one();
        promise_.set_value(std::move(value));
        read_.wait(lock, [this]() { return cur_size_ == 0; });
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> lock(global_);
        write_.wait(lock, [this]() { return cur_size_ || is_close_; });

        if (cur_size_ == 0 && is_close_) {
            return std::nullopt;
        }

        auto cur_elem = promise_.get_future();
        promise_ = std::promise<T>();
        --cur_size_;
        read_.notify_all();
        return cur_elem.get();
    }

    void Close() {
        std::unique_lock<std::mutex> lock(global_);
        is_close_ = true;
        write_.notify_all();
    }

private:
    std::mutex global_;
    std::promise<T> promise_;
    std::condition_variable read_;
    std::condition_variable write_;
    bool is_close_ = false;
    int cur_size_ = 0;
    const int max_size_ = 1;
};
