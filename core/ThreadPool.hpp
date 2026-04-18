#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "OrderBook.hpp"
#include "concurrentqueue.h"

namespace FastLittleMarket {

static constexpr size_t NUM_SHARDS = 4;

class ThreadPool {
 private:
  moodycamel::ConcurrentQueue<std::function<void()>> tasks_[NUM_SHARDS];
  std::array<std::jthread, NUM_SHARDS> workers_;

  std::atomic<bool> stop_{false};

 public:
  ThreadPool();
  ~ThreadPool() = default;

  void enqueue(std::function<void()> task, size_t shard_id);
};

}  // namespace FastLittleMarket