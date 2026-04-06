#include "OrderBook.hpp"

namespace FastLittleMarket {

bool OrderBook::priceCrosses(const Order& incoming) const {
  auto& opposite =
      (incoming.getSide() == BuyOrSell::buy) ? sell_orders_ : buy_orders_;
  if (opposite.empty()) return false;

  double opp_price = opposite.top().getPrice();
  return (incoming.getSide() == BuyOrSell::buy)
             ? (opp_price <= incoming.getPrice())
             : (opp_price >= incoming.getPrice());
}

void OrderBook::addOrder(const Order& order) {
  if (priceCrosses(order)) {
    matchOrder(order);
  } else {
    auto& own_queue =
        (order.getSide() == BuyOrSell::buy) ? buy_orders_ : sell_orders_;
    own_queue.push(order);
  }
}

bool OrderBook::priceCrosses(const Order& incoming,
                             const Order& opposite) const {
  return (incoming.getSide() == BuyOrSell::buy)
             ? (opposite.getPrice() <= incoming.getPrice())
             : (opposite.getPrice() >= incoming.getPrice());
}

void OrderBook::matchOrder(Order incoming) {
  auto& opposite_queue =
      (incoming.getSide() == BuyOrSell::buy) ? sell_orders_ : buy_orders_;

  while (incoming.isValid() && !opposite_queue.empty()) {
    Order best_opposite = opposite_queue.top();
    opposite_queue.pop();

    if (!priceCrosses(incoming, best_opposite)) break;

    int trade_volume =
        std::min(incoming.getVolume(), best_opposite.getVolume());

    // TODO: Log trade
    // logTrade(incoming, best_opposite, trade_volume);

    incoming.setVolume(incoming.getVolume() - trade_volume);
    best_opposite.setVolume(best_opposite.getVolume() - trade_volume);

    if (best_opposite.isValid()) {
      opposite_queue.push(best_opposite);
    }
  }

  if (incoming.isValid()) {
    auto& own_queue =
        (incoming.getSide() == BuyOrSell::buy) ? buy_orders_ : sell_orders_;
    own_queue.push(std::move(incoming));
  }
}

void OrderBook::cancelOrder(int order_id) {
  // TODO
  // Remove order from XXX_orders_ and account for decrease in volume
}

void OrderBook::modifyOrder(int order_id, double new_price, int new_volume) {
  // TODO
  // Remove order from XXX_orders_ and account for decrease in volume
  // If modify is removal, call ->cancelOrder
}

std::pair<Order, Order> OrderBook::getTopOfBook() const {
  // TODO return best sell and best buy orders
}

}  // namespace FastLittleMarket