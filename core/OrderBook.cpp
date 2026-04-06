#include "OrderBook.hpp"

namespace FastLittleMarket {

bool OrderBook::canCross(const Order& incoming) const {
  const OrderQueueInterface* opposite =
      (incoming.getSide() == BuyOrSell::buy)
          ? static_cast<const OrderQueueInterface*>(&sell_orders_)
          : static_cast<const OrderQueueInterface*>(&buy_orders_);

  if (opposite->empty() || !opposite->top().has_value()) return false;

  return priceCrosses(incoming, opposite->top().value());
}

bool OrderBook::priceCrosses(const Order& incoming,
                             const Order& opposite) const {
  return (incoming.getSide() == BuyOrSell::buy)
             ? (opposite.getPrice() <= incoming.getPrice())
             : (opposite.getPrice() >= incoming.getPrice());
}

void OrderBook::addOrder(const Order& order) {
  if (canCross(order)) {
    matchOrder(order);
  } else {
    auto* own_queue = (order.getSide() == BuyOrSell::buy)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);
    own_queue->push(order);
  }
}

void OrderBook::matchOrder(Order incoming) {
  auto* opposite_queue = (incoming.getSide() == BuyOrSell::buy)
                             ? static_cast<OrderQueueInterface*>(&sell_orders_)
                             : static_cast<OrderQueueInterface*>(&buy_orders_);

  while (incoming.isValid() && !opposite_queue->empty()) {
    Order best_opposite = opposite_queue->top().value();
    opposite_queue->pop();

    if (!priceCrosses(incoming, best_opposite)) break;

    int trade_volume =
        std::min(incoming.getVolume(), best_opposite.getVolume());

    // TODO: Log trade

    incoming.setVolume(incoming.getVolume() - trade_volume);
    best_opposite.setVolume(best_opposite.getVolume() - trade_volume);

    if (best_opposite.isValid()) {
      opposite_queue->push(best_opposite);
    }
  }

  if (incoming.isValid()) {
    auto* own_queue = (incoming.getSide() == BuyOrSell::buy)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);
    own_queue->push(std::move(incoming));
  }
}

void OrderBook::cancelOrder(int order_id) {
  if (orders_.find(order_id) != orders_.end()) {
    auto order = orders_.at(order_id);

    auto* price_queue = (order.getSide() == BuyOrSell::buy)
                            ? static_cast<OrderQueueInterface*>(&buy_orders_)
                            : static_cast<OrderQueueInterface*>(&sell_orders_);

    price_queue->remove(order.getId());
    volumes_[std::make_pair(order.getPrice(), order.getSide())] -=
        order.getVolume();
    orders_.erase(order_id);
  }
}

TopOfBook OrderBook::getTopOfBook() const {
  return {buy_orders_.top(), sell_orders_.top()};
}

}  // namespace FastLittleMarket