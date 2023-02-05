#pragma once

#include <mutex>
#include <condition_variable>

class DefaultCallback {
public:
    void operator()(int& value) {
        --value;
    }
};

class Semaphore {
public:
    Semaphore(int count) : count_(count) {
    }

    void Leave() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++count_;
        cv_.notify_all();
    }

    template <class Func>
    void Enter(Func callback) {
        std::unique_lock<std::mutex> lock(mutex_);
        int cur_thread = cur_max_;
        ++cur_max_;
        while (!count_ || cur_thread != cur_pos_) {
            cv_.wait(lock);
        }
        ++cur_pos_;
        callback(count_);
    }

    void Enter() {
        DefaultCallback callback;
        Enter(callback);
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int count_ = 0;
    int cur_max_ = 0;
    int cur_pos_ = 0;
};
