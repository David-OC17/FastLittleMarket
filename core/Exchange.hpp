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

  void worker(size_t shard_id) {
    while (!stop_) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        condition_.wait(lock, [this, shard_id] {
          return stop_ || !tasks_[shard_id].empty();
        });

        if (stop_ && tasks_[shard_id].empty()) return;

        task = std::move(tasks_[shard_id].front());
        tasks_[shard_id].pop();
      }
      task();
    }
  }

 public:
  ThreadPool() {
    for (size_t i = 0; i < NUM_SHARDS; ++i) {
      workers_[i] = std::jthread(&ThreadPool::worker, this, i);
    }
  }

  ~ThreadPool() {
    stop_ = true;
    condition_.notify_all();
  }

  void enqueue(std::function<void()> task, size_t shard_id) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      tasks_[shard_id].push(std::move(task));
    }
    condition_.notify_one();
  }
};

class Exchange {
 private:
  std::array<std::unordered_map<std::string, OrderBook>, NUM_SHARDS> shards_;
  ThreadPool thread_pool_;

 public:
  static Exchange& getInstance();

  size_t getShard(const std::string& symbol) const;
  void addOrder(const std::string& symbol, Order order);
  void cancelOrder(const std::string& symbol, int orderId);
};

}  // namespace FastLittleMarket