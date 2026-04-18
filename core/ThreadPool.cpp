#include "ThreadPool.hpp"

#include "Symbols.hpp"

namespace FastLittleMarket {

ThreadPool::ThreadPool() {
  for (size_t i = 0; i < NUM_SHARDS; ++i) {
    workers_[i] = std::jthread([this, i](std::stop_token st) {
      while (!st.stop_requested()) {
        std::function<void()> task;
        if (tasks_[i].try_dequeue(task)) {
          task();
        } else {
          std::this_thread::yield();
        }
      }
    });
  }
}

void ThreadPool::enqueue(std::function<void()> task, size_t shard_id) {
  tasks_[shard_id].enqueue(std::move(task));
}

}  // namespace FastLittleMarket