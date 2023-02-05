#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <map>
#include <set>
#include <deque>

template <class T>
class TimerQueue {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

public:
    void Add(const T& item, TimePoint at) {
        std::unique_lock<std::mutex> mtx(mtx_);
        auto search_res = data_.find(at);

        if (search_res != data_.end()) {
            data_[at].push_back(item);
        } else {
            std::deque<T> temp{item};
            stamps_.insert(at);
            data_[at] = temp;
        }

        cv_.notify_one();
    }

    T Pop() {
        std::unique_lock<std::mutex> guard{mtx_};
        cv_.wait(guard, [this]() { return !stamps_.empty(); });
        cv_.wait_until(guard, *stamps_.begin());

        auto cur_clock = *stamps_.begin();

        if (data_[cur_clock].size() > 1) {
            auto value = std::move(data_[cur_clock].front());
            data_[cur_clock].pop_front();

            return value;
        } else {
            auto value = std::move(data_[cur_clock].front());
            data_.erase(cur_clock);
            stamps_.erase(*stamps_.begin());

            return value;
        }
    }

private:
    std::map<TimePoint, std::deque<T>> data_;
    std::set<TimePoint> stamps_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
