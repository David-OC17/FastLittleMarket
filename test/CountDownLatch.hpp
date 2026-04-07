#include <gtest/gtest.h>
#include <future>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

#include "Exchange.hpp"

class CountDownLatch {
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int count_;
    
public:
    explicit CountDownLatch(int count) : count_(count) {}
    
    void countDown() {
        std::unique_lock<std::mutex> lock(mutex_);
        if (--count_ == 0) cv_.notify_all();
    }
    
    template<typename Rep, typename Period>
    bool await(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this] { return count_ == 0; });
    }
};