#pragma once

#include "indexforge/types.hpp"
#include <string>
#include <list>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <atomic>

namespace indexforge {

class QueryCache {
public:
    explicit QueryCache(size_t capacity = 1000)
        : capacity_(capacity) {}

    std::optional<SearchResult> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end()) {
            misses_++;
            return std::nullopt;
        }

        hits_++;
        // Move accessed item to front of LRU list
        order_.splice(order_.begin(), order_, it->second);
        auto result = it->second->second;
        result.from_cache = true;
        return result;
    }

    void put(const std::string& key, const SearchResult& result) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = result;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }

        if (order_.size() >= capacity_) {
            // Evict least recently used (back of order_)
            const auto& back_key = order_.back().first;
            map_.erase(back_key);
            order_.pop_back();
        }

        order_.emplace_front(key, result);
        map_[key] = order_.begin();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        order_.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    size_t hits() const { return hits_.load(); }
    size_t misses() const { return misses_.load(); }

private:
    size_t capacity_;
    mutable std::mutex mutex_;
    using CacheEntry = std::pair<std::string, SearchResult>;
    std::list<CacheEntry> order_;
    std::unordered_map<std::string, std::list<CacheEntry>::iterator> map_;

    std::atomic<size_t> hits_{0};
    std::atomic<size_t> misses_{0};
};

} // namespace indexforge
