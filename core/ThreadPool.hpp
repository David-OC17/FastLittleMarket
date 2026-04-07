#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "OrderBook.hpp"

namespace FastLittleMarket {

static constexpr size_t NUM_SHARDS = 4;

class ThreadPool {
 private:
  std::array<std::jthread, NUM_SHARDS> workers_;
  std::array<std::queue<std::function<void()>>, NUM_SHARDS> tasks_;
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_{false};

  void worker(size_t shard_id);

 public:
  ThreadPool();
  ~ThreadPool();

  void enqueue(std::function<void()> task, size_t shard_id);
};

}  // namespace FastLittleMarket