#pragma once
#include <mutex>
#include <condition_variable>

class RWLock {
public:
    template <class Func>
    void Read(Func func) {
        std::unique_lock<std::mutex> lock(global_mtx_);
        ++status_;
        read_cv_.wait(lock, [this]() { return status_; });
        lock.unlock();
        lock.release();

        try {
            func();
        } catch (...) {
            EndRead();
            throw;
        }
        EndRead();
    }

    template <class Func>
    void Write(Func func) {
        std::unique_lock<std::mutex> lock(global_mtx_);
        write_cv_.wait(lock, [this]() { return status_ == 0; });
        read_cv_.notify_all();
        write_cv_.notify_one();
        func();
    }

private:
    std::mutex global_mtx_;
    std::condition_variable read_cv_;
    std::condition_variable write_cv_;
    int status_ = 0;

    void EndRead() {
        global_mtx_.lock();
        --status_;

        if (status_ == 0) {
            write_cv_.notify_one();
        } else {
            read_cv_.notify_all();
        }
        global_mtx_.unlock();
    }
};
