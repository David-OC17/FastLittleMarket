#include "OrderBook.hpp"

namespace FastLittleMarket {

bool OrderBook::canCross(const Order& incoming) const {
  const OrderQueueInterface* opposite =
      (incoming.getSide() == OrderSide::Buy)
          ? static_cast<const OrderQueueInterface*>(&sell_orders_)
          : static_cast<const OrderQueueInterface*>(&buy_orders_);

  if (opposite->empty() || !opposite->top().has_value()) return false;

  return priceCrosses(incoming, opposite->top().value());
}

bool OrderBook::priceCrosses(const Order& incoming,
                             const Order& opposite) const {
  return (incoming.getSide() == OrderSide::Buy)
             ? (opposite.getPrice() <= incoming.getPrice())
             : (opposite.getPrice() >= incoming.getPrice());
}

void OrderBook::addOrder(const Order& order) {
  if (canCross(order)) {
    matchOrder(order);
  } else {
    auto* own_queue = (order.getSide() == OrderSide::Buy)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);
    own_queue->push(order);
    volumes_[std::make_pair(order.getPrice(), order.getSide())] +=
        order.getVolume();
  }
}

void OrderBook::matchOrder(Order incoming) {
  auto* opposite_queue = (incoming.getSide() == OrderSide::Buy)
                             ? static_cast<OrderQueueInterface*>(&sell_orders_)
                             : static_cast<OrderQueueInterface*>(&buy_orders_);

  while (incoming.isValid() && !opposite_queue->empty()) {
    Order best_opposite = opposite_queue->top().value();

    if (!priceCrosses(incoming, best_opposite)) break;
    opposite_queue->pop();

    const int trade_volume =
        std::min(incoming.getVolume(), best_opposite.getVolume());

    // TODO: Log trade

    incoming.setVolume(incoming.getVolume() - trade_volume);
    best_opposite.setVolume(best_opposite.getVolume() - trade_volume);

    if (best_opposite.isValid()) {
      opposite_queue->push(best_opposite);
    }
  }

  if (incoming.isValid()) {
    auto* own_queue = (incoming.getSide() == OrderSide::Buy)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);
    own_queue->push(std::move(incoming));
  }
}

bool OrderBook::cancelOrder(int order_id) {
  if (auto it = buy_orders_.find(order_id); it.has_value()) {
    auto order = it.value();

    if (!buy_orders_.remove(order_id)) return false;

    volumes_[{order.getPrice(), OrderSide::Buy}] -= order.getVolume();
    return true;
  }

  else if (auto it = sell_orders_.find(order_id); it.has_value()) {
    auto order = it.value();

    if (!sell_orders_.remove(order_id)) return false;

    volumes_[{order.getPrice(), OrderSide::Sell}] -= order.getVolume();
    return true;
  }

  return false;
}

TopOfBook OrderBook::getTopOfBook() const {
  return {buy_orders_.top(), sell_orders_.top()};
}

}  // namespace FastLittleMarket