#pragma once

#include <atomic>

struct RWSpinLock {
private:
    std::atomic<unsigned> state_;
    const unsigned mask_ = 1 << 31;

public:
    void LockRead() {
        while (state_.fetch_add(1) & mask_) {
            UnlockRead();
        }
    }

    void UnlockRead() {
        state_.fetch_sub(1);
    }

    void LockWrite() {
        unsigned prev_val;

        while ((prev_val = state_.fetch_or(mask_)) != 0) {
            if ((prev_val & mask_) == 0) {  // count of readers > 0
                UnlockWrite();
            }
        }
    }

    void UnlockWrite() {
        state_.fetch_and(~mask_);
    }
};
