#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <atomic>

namespace helab {

/// Thread-safe queue for producer-consumer pattern
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    ThreadSafeQueue(ThreadSafeQueue&&) = default;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = default;

    /// Push an item to the queue
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        condVar_.notify_one();
    }

    /// Push multiple items to the queue
    template<typename Iterator>
    void push(Iterator begin, Iterator end) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto it = begin; it != end; ++it) {
                queue_.push(std::move(*it));
            }
        }
        condVar_.notify_all();
    }

    /// Pop an item from the queue (blocking)
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condVar_.wait(lock, [this] { return !queue_.empty() || stopped_; });

        if (stopped_ && queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /// Pop an item with timeout
    template<typename Rep, typename Period>
    std::optional<T> popFor(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!condVar_.wait_for(lock, timeout, [this] { return !queue_.empty() || stopped_; })) {
            return std::nullopt;  // Timeout
        }

        if (stopped_ && queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /// Try to pop without blocking
    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /// Get the front item without removing it
    std::optional<T> peek() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        return queue_.front();
    }

    /// Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /// Get queue size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /// Clear the queue
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }

    /// Stop the queue (unblocks all waiting threads)
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        condVar_.notify_all();
    }

    /// Reset the queue for reuse
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = false;
        std::queue<T> empty;
        std::swap(queue_, empty);
    }

    /// Check if the queue is stopped
    bool isStopped() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

    /// Get maximum size (0 = unlimited)
    size_t maxSize() const {
        return maxSize_;
    }

    /// Set maximum size (0 = unlimited)
    void setMaxSize(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxSize_ = size;
    }

    /// Check if queue is full
    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxSize_ > 0 && queue_.size() >= maxSize_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condVar_;
    std::queue<T> queue_;
    std::atomic<bool> stopped_{false};
    size_t maxSize_ = 0;  // 0 = unlimited
};

} // namespace helab
