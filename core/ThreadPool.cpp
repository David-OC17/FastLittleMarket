#include "ThreadPool.hpp"

#include <cassert>

#include "Symbols.hpp"

namespace FastLittleMarket {

void ThreadPool::worker(size_t shard_id) {
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

ThreadPool::ThreadPool() {
  for (size_t i = 0; i < NUM_SHARDS; ++i) {
    workers_[i] = std::jthread(&ThreadPool::worker, this, i);
  }
}

ThreadPool::~ThreadPool() {
  stop_ = true;
  condition_.notify_all();
}

void ThreadPool::enqueue(std::function<void()> task, size_t shard_id) {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    tasks_[shard_id].push(std::move(task));
  }
  condition_.notify_one();
}

}  // namespace FastLittleMarket