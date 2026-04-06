#pragma once

#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Order.hpp"

namespace FastLittleMarket {

using SellOrderQueue =
    std::priority_queue<Order, std::vector<Order>, SellOrderComparator>;
using BuyOrderQueue =
    std::priority_queue<Order, std::vector<Order>, BuyOrderComparator>;

struct BuyOrderComparator {
  bool operator()(const Order& a, const Order& b) const {
    if (a.getPrice() == b.getPrice()) {
      return a.getTimestamp() > b.getTimestamp();
    }
    return a.getPrice() < b.getPrice();
  }
};

struct SellOrderComparator {
  bool operator()(const Order& a, const Order& b) const {
    if (a.getPrice() == b.getPrice()) {
      return a.getTimestamp() > b.getTimestamp();
    }
    return a.getPrice() > b.getPrice();
  }
};

class OrderBook {
 public:
  void addOrder(const Order& order);
  void cancelOrder(int order_id);
  void modifyOrder(int order_id, double new_price, int new_volume);

  std::pair<Order, Order> getTopOfBook() const;

  Order getOrder(int order_id);

 private:
  bool priceCrosses(const Order& incoming) const;
  bool priceCrosses(const Order& incoming, const Order& opposite) const;

  SellOrderQueue sell_orders_;
  BuyOrderQueue buy_orders_;

  std::unordered_map<int, Order> orders_;
  std::unordered_map<std::pair<double, BuyOrSell>, int> volumes_;
  std::unordered_map<std::pair<double, BuyOrSell>, int> TODO;  // TODO

  bool OrderBook::priceCrosses(const Order& incoming) const;
  void OrderBook::priceCrosses(const Order& incoming,
                               const Order& opposite) const;
  void matchOrder(Order incoming);
};

}  // namespace FastLittleMarket