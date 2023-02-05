#pragma once

#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>

// Atomically do the following:
//    if (*value == expected_value) {
//        sleep_on_address(value)
//    }
void FutexWait(int *value, int expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value, nullptr, nullptr, 0);
}

// Wakeup 'count' threads sleeping on address of value(-1 wakes all)
void FutexWake(int *value, int count) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

class Mutex {
    int CompareExchanger(int expected, int desired) {
        int sucess_value = expected;
        atomic_state_.compare_exchange_strong(sucess_value, desired);

        return sucess_value;
    }

public:
    void Lock() {
        int compare_res = CompareExchanger(0, 1);

        if (compare_res != 0) {  // mtx was locked
            do {
                if (compare_res == 2 || CompareExchanger(1, 2) != 0) {
                    FutexWait(&state_, 2);  // wait until state == 2
                }
            } while ((compare_res = CompareExchanger(0, 2)) != 0);
        }
    }

    void Unlock() {
        if (atomic_state_.fetch_sub(1) != 1) {
            atomic_state_.store(0);

            FutexWake(&state_, 1);
        }
    }

private:
    int state_ = 0;
    std::atomic_ref<int> atomic_state_{state_};
    // 0 - unlocked; 1 - lock with waiters; 2 - lock without waiters;
};