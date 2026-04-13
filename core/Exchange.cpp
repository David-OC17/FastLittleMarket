#include <Exchange.hpp>
#include <cassert>

#include "Symbols.hpp"

namespace FastLittleMarket {

Exchange& Exchange::getInstance() {
  thread_local Exchange inst;

  return inst;
}

size_t Exchange::getShard(const std::string& symbol) const {
  return std::hash<std::string>{}(symbol) % NUM_SHARDS;
}

void Exchange::addOrder(const std::string& symbol, Order order) {
  size_t shard = getShard(symbol);
  thread_pool_.enqueue(
      [this, symbol, order, shard] {
        auto& book = shards_[shard][symbol];
        book.addOrder(order);
      },
      shard);
}

void Exchange::cancelOrder(const std::string& symbol, uint64_t orderId) {
  size_t shard = getShard(symbol);
  thread_pool_.enqueue(
      [this, symbol, orderId, shard] {
        auto it = shards_[shard].find(symbol);
        if (it != shards_[shard].end()) {
          it->second.cancelOrder(orderId);
        }
      },
      shard);
}

std::optional<Order> Exchange::getOrder(const std::string& symbol,
                                        uint64_t orderId) const {
  size_t shard = getShard(symbol);

  auto it = shards_[shard].find(symbol);
  if (it != shards_[shard].end()) {
    return it->second.getOrder(orderId);
  }

  return std::nullopt;
}

}  // namespace FastLittleMarket