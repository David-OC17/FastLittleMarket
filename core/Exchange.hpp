#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "OrderBook.hpp"
#include "ThreadPool.hpp"

namespace FastLittleMarket {

class Exchange {
 private:
  std::array<std::unordered_map<std::string, OrderBook>, NUM_SHARDS> shards_;
  ThreadPool thread_pool_;

 public:
  static Exchange& getInstance();

  size_t getShard(const std::string& symbol) const;
  void addOrder(const std::string& symbol, Order order);
  void cancelOrder(const std::string& symbol, int orderId);

  std::optional<Order> getOrder(const std::string& symbol, int orderId) const;
};

}  // namespace FastLittleMarket