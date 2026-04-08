#pragma once

#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Order.hpp"
#include "GlobalSequencer.hpp"

namespace FastLittleMarket {

struct BuyOrderComparator {
  bool operator()(const Order& a, const Order& b) const {
    if (a.getPrice() == b.getPrice()) {
      return a.getTimestamp() > b.getTimestamp();
    }
    return a.getPrice() > b.getPrice();
  }
};

struct SellOrderComparator {
  bool operator()(const Order& a, const Order& b) const {
    if (a.getPrice() == b.getPrice()) {
      return a.getTimestamp() > b.getTimestamp();
    }
    return a.getPrice() < b.getPrice();
  }
};

struct PriceSideHash {
  std::size_t operator()(
      const std::pair<double, OrderSide>& key) const noexcept {
    std::size_t h1 = std::hash<double>{}(key.first);
    std::size_t h2 = std::hash<std::underlying_type_t<OrderSide>>{}(
        static_cast<std::underlying_type_t<OrderSide>>(key.second));
    return h1 ^ (h2 << 1);
  }
};

struct PriceSideEqual {
  bool operator()(const std::pair<double, OrderSide>& lhs,
                  const std::pair<double, OrderSide>& rhs) const noexcept {
    return lhs.first == rhs.first && lhs.second == rhs.second;
  }
};

class OrderQueueInterface {
 public:
  virtual ~OrderQueueInterface() = default;
  virtual bool empty() const = 0;
  virtual std::optional<Order> top() const = 0;
  virtual bool push(const Order& order) = 0;
  virtual std::optional<Order> find(int order_id) const = 0;
  virtual bool pop() = 0;
  virtual bool remove(int order_id) = 0;
  virtual bool isValid() const = 0;
};

template <typename Comparator>
class PriorityQueueAdapter : public OrderQueueInterface {
 private:
  std::multiset<Order, Comparator> price_sorted_;
  std::unordered_map<int, typename decltype(price_sorted_)::iterator>
      id_to_iter_;

 public:
  bool empty() const override { return price_sorted_.empty(); }

  std::optional<Order> top() const override {
    if (empty()) return std::nullopt;
    return *price_sorted_.begin();
  }

  bool push(const Order& order) override {
    if (!order.isValid()) return false;

    auto it = price_sorted_.insert(order);
    id_to_iter_[order.getId()] = it;
    return true;
  }

  std::optional<Order> find(int order_id) const {
    auto map_it = id_to_iter_.find(order_id);
    if (map_it == id_to_iter_.end()) {
      return std::nullopt;
    }
    return *(map_it->second);
  }

  bool pop() override {
    if (empty()) return false;

    int id = price_sorted_.begin()->getId();
    id_to_iter_.erase(id);
    price_sorted_.erase(price_sorted_.begin());
    return true;
  }

  bool remove(int order_id) override {
    auto map_it = id_to_iter_.find(order_id);
    if (map_it == id_to_iter_.end()) return false;

    price_sorted_.erase(map_it->second);
    id_to_iter_.erase(map_it);

    return true;
  }

  bool isValid() const override {
    for (const auto& order : price_sorted_) {
      if (!order.isValid()) return false;
    }
    return true;
  }
};

using BuyOrderQueue = PriorityQueueAdapter<BuyOrderComparator>;
using SellOrderQueue = PriorityQueueAdapter<SellOrderComparator>;

struct TopOfBook {
  std::optional<Order> bid;
  std::optional<Order> ask;

  bool hasBid() const { return bid.has_value(); }
  bool hasAsk() const { return ask.has_value(); }
  bool valid() const { return hasBid() && hasAsk(); }
};

class OrderBook {
 public:
  bool addOrder(const Order& order);
  bool cancelOrder(int order_id);
  // TODO modifyOrder()
  TopOfBook getTopOfBook() const;

  std::optional<Order> getOrder(int order_id) const;

 private:
  bool canCross(const Order& incoming) const;
  bool priceCrosses(const Order& incoming, const Order& opposite) const;

  SellOrderQueue sell_orders_;
  BuyOrderQueue buy_orders_;

  std::unordered_map<std::pair<double, OrderSide>, int, PriceSideHash,
                     PriceSideEqual>
      volumes_;

  bool matchOrder(Order incoming);
};

}  // namespace FastLittleMarket