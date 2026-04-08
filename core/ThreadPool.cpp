#include "ThreadPool.hpp"

#include "Symbols.hpp"

namespace FastLittleMarket {

void ThreadPool::worker(size_t shard_id) {
  while (!stop_) {
    std::function<void()> task;
    if (stop_) return;
    if (tasks_[shard_id].try_dequeue(task)) {
      task();
    }
  }
}

ThreadPool::ThreadPool() {
  for (size_t i = 0; i < NUM_SHARDS; ++i) {
    workers_[i] = std::jthread(&ThreadPool::worker, this, i);
  }
}

ThreadPool::~ThreadPool() { stop_ = true; }

void ThreadPool::enqueue(std::function<void()> task, size_t shard_id) {
  tasks_[shard_id].enqueue(std::move(task));
}

}  // namespace FastLittleMarket