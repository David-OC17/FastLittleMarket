#pragma once

#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "GlobalSequencer.hpp"
#include "Order.hpp"

namespace fast_little_market {

struct BuyOrderComparator {
  bool operator()(const Order& a, const Order& b) const {
    if (a.price_q4_ == b.price_q4_) {
      return Order::unpack(a.id_ns_).timestamp_ns >
             Order::unpack(b.id_ns_).timestamp_ns;  // FIFO by timestamp
    }
    return a.price_q4_ > b.price_q4_;
  }
};

struct SellOrderComparator {
  bool operator()(const Order& a, const Order& b) const {
    if (a.price_q4_ == b.price_q4_) {
      return Order::unpack(a.id_ns_).timestamp_ns >
             Order::unpack(b.id_ns_).timestamp_ns;  // FIFO by timestamp
    }
    return a.price_q4_ < b.price_q4_;
  }
};

struct PriceSideHash {
  std::size_t operator()(const std::pair<double, uint8_t>& key) const noexcept {
    std::size_t h1 = std::hash<double>{}(key.first);
    std::size_t h2 = std::hash<uint8_t>{}(key.second);
    return h1 ^ (h2 << 1);
  }
};

struct PriceSideEqual {
  bool operator()(const std::pair<uint32_t, uint8_t>& lhs,
                  const std::pair<uint32_t, uint8_t>& rhs) const noexcept {
    return lhs.first == rhs.first && lhs.second == rhs.second;
  }
};

class OrderQueueInterface {
 public:
  virtual ~OrderQueueInterface() = default;
  virtual bool empty() const = 0;
  virtual std::optional<Order> top() const = 0;
  virtual bool push(const Order& order) = 0;
  virtual std::optional<Order> find(uint64_t order_id) const = 0;
  virtual bool pop() = 0;
  virtual bool remove(uint64_t order_id) = 0;
  virtual bool isValid() const = 0;
};

template <typename Comparator>
class PriorityQueueAdapter : public OrderQueueInterface {
 private:
  std::multiset<Order, Comparator> price_sorted_;
  std::unordered_map<uint64_t, typename decltype(price_sorted_)::iterator>
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
    id_to_iter_[order.id_ns_] = it;
    return true;
  }

  std::optional<Order> find(uint64_t order_id) const override {
    auto map_it = id_to_iter_.find(order_id);
    if (map_it == id_to_iter_.end()) {
      return std::nullopt;
    }
    return *(map_it->second);
  }

  bool pop() override {
    if (empty()) return false;

    int id = price_sorted_.begin()->id_ns_;
    id_to_iter_.erase(id);
    price_sorted_.erase(price_sorted_.begin());
    return true;
  }

  bool remove(uint64_t order_id) override {
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

enum class ExecFlags : uint8_t {
  None = 0,
  Accepted = uint8_t(1) << 0,
  PartiallyFilled = uint8_t(1) << 1,
  FullyFilled = uint8_t(1) << 2,
  Cancelled = uint8_t(1) << 3,
  Rejected = uint8_t(1) << 4
};

inline ExecFlags operator|(ExecFlags a, ExecFlags b) {
  return static_cast<ExecFlags>(static_cast<uint8_t>(a) |
                                static_cast<uint8_t>(b));
}

inline ExecFlags operator&(ExecFlags a, ExecFlags b) {
  return static_cast<ExecFlags>(static_cast<uint8_t>(a) &
                                static_cast<uint8_t>(b));
}

inline ExecFlags& operator|=(ExecFlags& a, ExecFlags b) {
  a = a | b;
  return a;
}

inline bool hasFlag(ExecFlags flags, ExecFlags f) {
  return (flags & f) != ExecFlags::None;
}

inline bool isValid(ExecFlags f) {
  // Rejected is exclusive
  if (hasFlag(f, ExecFlags::Rejected)) return f == ExecFlags::Rejected;
  // Can't be both partially and fully filled
  if (hasFlag(f, ExecFlags::PartiallyFilled) &&
      hasFlag(f, ExecFlags::FullyFilled))
    return false;
  // Cancelled and Accepted together is suspect
  if (hasFlag(f, ExecFlags::Cancelled) && hasFlag(f, ExecFlags::Accepted))
    return false;
  return true;
}

class OrderBook {
 public:
  ExecFlags newOrder(const Order& order);
  ExecFlags cancelOrder(uint64_t order_id);
  ExecFlags modifyOrder(uint64_t order_id, uint32_t new_price_q4,
                        uint32_t new_volume, GlobalSequencer& sequencer);
  TopOfBook getTopOfBook() const;

  std::optional<Order> getOrder(uint64_t order_id) const;

 private:
  bool canCross(const Order& incoming) const;
  bool priceCrosses(const Order& incoming, const Order& opposite) const;

  SellOrderQueue sell_orders_;
  BuyOrderQueue buy_orders_;

  std::unordered_map<std::pair<double, uint8_t>, int, PriceSideHash,
                     PriceSideEqual>
      volumes_;

  ExecFlags matchOrder(Order incoming);
};

}  // namespace fast_little_market