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
  std::array<std::jthread, NUM_SHARDS> workers_;
  moodycamel::ConcurrentQueue<std::function<void()>> tasks_[NUM_SHARDS];

  std::atomic<bool> stop_{false};

  void worker(size_t shard_id);

 public:
  ThreadPool();
  ~ThreadPool();

  void enqueue(std::function<void()> task, size_t shard_id);
};

}  // namespace FastLittleMarket