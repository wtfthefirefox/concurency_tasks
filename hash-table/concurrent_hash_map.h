#pragma once

#include <unordered_map>
#include <mutex>
#include <functional>
#include <thread>
#include <vector>
#include <deque>

// size / thread_count <= 0.7
template <class K, class V, class Hash = std::hash<K>>
class ConcurrentHashMap {
public:
    ConcurrentHashMap(const Hash& hasher = Hash())
        : mutex_count_(kDefaultConcurrencyLevel),
          hash_func_(hasher),
          segment_size_(kDefaultConcurrencyLevel) {
        data_.resize(segment_size_);

        auto reserved_value = kUndefinedSize / kDefaultConcurrencyLevel + 1;

        for (auto& el : data_) {
            el.reserve(reserved_value);
        }

        for (int i = 0; i < segment_size_; ++i) {
            mutex_list_.emplace_back();
        }
    }

    explicit ConcurrentHashMap(int expected_size, const Hash& hasher = Hash())
        : hash_func_(hasher) {

        int new_concurrency = kDefaultConcurrencyLevel;

        while (expected_size > new_concurrency) {
            new_concurrency *= 2;
        }

        segment_size_ = new_concurrency;
        mutex_count_ = segment_size_;
        data_.resize(segment_size_);

        for (int i = 0; i < segment_size_; ++i) {
            mutex_list_.emplace_back();
        }
    }

    ConcurrentHashMap(int expected_size, int expected_threads_count, const Hash& hasher = Hash())
        :  // 0 because unordered_map doesn't contain constructor with hasher
          hash_func_(hasher) {
        int new_concurrency = 1;
        while (new_concurrency < expected_threads_count) {
            new_concurrency <<= 1;
        }

        while (expected_size > new_concurrency) {
            new_concurrency *= 2;
        }

        segment_size_ = new_concurrency;
        mutex_count_ = segment_size_;
        data_.resize(segment_size_);

        for (int i = 0; i < segment_size_; ++i) {
            mutex_list_.emplace_back();
        }
    }

    bool Insert(const K& key, const V& value) {
        std::unique_lock locker(mutex_list_[GetMutexIdx(key)]);
        size_t idx = GetIdx(key);

        for (size_t i = 0; i < data_[idx].size(); ++i) {
            if (data_[idx][i].first == key) {
                return false;
            }
        }
        data_[idx].push_back({key, value});
        size_ += 1;

        if (size_ > 0.7 * segment_size_) {  // load factor
            locker.unlock();
            ReHasher();
        }

        return true;
    }

    bool Erase(const K& key) {
        std::unique_lock locker(mutex_list_[GetMutexIdx(key)]);
        size_t idx = GetIdx(key);

        bool is_founded = false;

        for (auto it = std::begin(data_[idx]); it != std::end(data_[idx]); ++it) {
            if ((*it).first == key) {
                is_founded = true;
                data_[idx].erase(it);
                break;
            }
        }

        if (is_founded) {
            size_ -= 1;
            return true;
        }

        return false;
    }

    void Clear() {
        LockAllMutex();
        for (auto& el : data_) {
            el.clear();
        }
        size_ = 0;
        UnlockAllMutex();
    }

    std::pair<bool, V> Find(const K& key) {
        std::unique_lock locker(mutex_list_[GetMutexIdx(key)]);
        size_t idx = GetIdx(key);

        auto res_pair = std::make_pair(false, V());

        for (const auto& el : data_[idx]) {
            if (el.first == key) {
                res_pair = std::make_pair(true, el.second);
                break;
            }
        }

        return res_pair;
    }

    std::pair<bool, V> Find(const K& key) const {
        size_t idx = GetIdx(key);
        auto res_pair = std::make_pair(false, V());

        for (const auto& el : data_[idx]) {
            if (el.first == key) {
                res_pair = std::make_pair(true, el.second);
                break;
            }
        }

        return res_pair;
    }

    const V At(const K& key) const {
        int hash = hash_func_(key);
        int idx = hash % segment_size_;

        for (const auto& el : data_[idx]) {
            if (el.first == key) {
                return el.second;
            }
        }

        throw std::out_of_range("This key does not exist");
    }

    size_t Size() const {
        return size_;
    }

    static const int kDefaultConcurrencyLevel;
    static const int kUndefinedSize;

private:
    std::vector<std::vector<std::pair<K, V>>> data_;
    std::deque<std::mutex> mutex_list_;
    size_t mutex_count_ = 0;
    Hash hash_func_ = std::hash<K>();
    std::atomic<int> segment_size_ = kDefaultConcurrencyLevel;
    std::atomic<int> size_ = 0;

    size_t GetIdx(K key) const {
        return hash_func_(key) % segment_size_;
    }

    size_t GetMutexIdx(K key) const {
        return GetIdx(key) % mutex_count_;
    }

    void LockAllMutex() {
        for (auto& mtx : mutex_list_) {
            mtx.lock();
        }
    }

    void UnlockAllMutex() {
        for (int i = mutex_count_ - 1; i >= 0; --i) {
            mutex_list_[i].unlock();
        }
    }

    void ReHasher() {
        LockAllMutex();

        if (size_ <= 0.7 * segment_size_) {
            UnlockAllMutex();  // rehash has been done
        } else {
            int expected_size = size_ * 2;

            while (expected_size > segment_size_) {
                segment_size_ += segment_size_;
            }

            std::vector<std::vector<std::pair<K, V>>> new_data(segment_size_);

            for (const auto& list_item : data_) {
                for (const auto& item : list_item) {
                    size_t cur_idx = hash_func_(item.first) % segment_size_;
                    new_data[cur_idx].push_back(std::make_pair(item.first, item.second));
                }
            }

            std::swap(data_, new_data);

            UnlockAllMutex();
        }
    }
};

template <class K, class V, class Hash>
const int ConcurrentHashMap<K, V, Hash>::kDefaultConcurrencyLevel =
    std::thread::hardware_concurrency();

template <class K, class V, class Hash>
const int ConcurrentHashMap<K, V, Hash>::kUndefinedSize = 100;
