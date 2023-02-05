#pragma once

#include <atomic>
#include <vector>
#include <stdexcept>
#include <optional>

template <class T>
class MPMCBoundedQueue {
    struct QueueElem {
        std::atomic<unsigned> gen;
        std::optional<T> data;
    };

public:
    explicit MPMCBoundedQueue(int size) : data_(size), mask_(size - 1) {
        if ((size & (size - 1)) != 0) {
            throw std::runtime_error("size must be % 2");
        }

        for (int i = 0; i < size; ++i) {
            data_[i].gen = i;
        }
    }

    bool Enqueue(const T& value) {
        unsigned idx_write = write_.load();

        while (true) {
            unsigned pos = idx_write & mask_;
            unsigned cur_gen = data_[pos].gen.load();

            if (cur_gen == idx_write) {
                if (write_.compare_exchange_weak(idx_write, idx_write + 1)) {
                    data_[pos].data = value;
                    data_[pos].gen = cur_gen + 1;

                    return true;
                }
            } else if (idx_write > cur_gen) {
                return false;
            } else {
                idx_write = write_.load();
            }
        }
    }

    bool Dequeue(T& data) {
        unsigned idx_read = read_.load();
        unsigned idx_write = write_.load();

        while (true) {
            unsigned pos = idx_read & mask_;
            unsigned cur_gen = data_[pos].gen.load();

            if (cur_gen == idx_read + 1) {
                if (read_.compare_exchange_weak(idx_read, idx_read + 1)) {
                    data = data_[pos].data.value();
                    data_[pos].data = std::nullopt;
                    data_[pos].gen = idx_read + 1 + mask_;

                    return true;
                }
            } else if (idx_write == idx_read) {
                return false;
            } else {
                idx_write = write_.load();
                idx_read = read_.load();
            }
        }
    }

private:
    std::vector<QueueElem> data_;
    unsigned mask_;
    std::atomic<unsigned> write_ = 0;
    std::atomic<unsigned> read_ = 0;
};