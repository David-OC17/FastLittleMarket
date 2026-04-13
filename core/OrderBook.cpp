#include "OrderBook.hpp"

#include <cassert>

namespace FastLittleMarket {

bool OrderBook::canCross(const Order& incoming) const {
  const OrderQueueInterface* opposite =
      (incoming.side == BUY_SIDE)
          ? static_cast<const OrderQueueInterface*>(&sell_orders_)
          : static_cast<const OrderQueueInterface*>(&buy_orders_);

  if (opposite->empty() || !opposite->top().has_value()) return false;

  return priceCrosses(incoming, opposite->top().value());
}

bool OrderBook::priceCrosses(const Order& incoming,
                             const Order& opposite) const {
  return (incoming.side == BUY_SIDE)
             ? (opposite.price_q4 <= incoming.price_q4)
             : (opposite.price_q4 >= incoming.price_q4);
}

// TODO: change return type to reflect if order was fully matched, partially matched, or added to book
bool OrderBook::addOrder(const Order& order) {
  if (canCross(order)) {
    if (!matchOrder(order)) {
      return false;
    }

  } else {
    auto* own_queue = (order.side == BUY_SIDE)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);

    assert(own_queue->isValid());

    own_queue->push(order);
    volumes_[std::make_pair(order.price_q4, order.side)] +=
        order.volume;
  }

  return true;
}

bool OrderBook::matchOrder(Order incoming) {
  if (!incoming.isValid()) return false;

  auto* opposite_queue = (incoming.side == BUY_SIDE)
                             ? static_cast<OrderQueueInterface*>(&sell_orders_)
                             : static_cast<OrderQueueInterface*>(&buy_orders_);

  assert(opposite_queue->isValid());

  if (opposite_queue->empty()) return false;

  while (incoming.isValid() && !opposite_queue->empty()) {
    Order best_opposite = opposite_queue->top().value();

    if (!priceCrosses(incoming, best_opposite)) break;
    opposite_queue->pop();

    const int trade_volume =
        std::min(incoming.volume, best_opposite.volume);

    incoming.volume -= trade_volume;
    best_opposite.volume -= trade_volume;

    if (best_opposite.isValid()) {
      opposite_queue->push(best_opposite);
    }
  }

  if (incoming.isValid()) {
    auto* own_queue = (incoming.side == BUY_SIDE)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);
    assert(own_queue->isValid());
    own_queue->push(std::move(incoming));
  }

  return true;
}

bool OrderBook::cancelOrder(uint64_t order_id) {
  if (auto it = buy_orders_.find(order_id); it.has_value()) {
    auto order = it.value();

    if (!buy_orders_.remove(order_id)) return false;

    volumes_[{order.price_q4, BUY_SIDE}] -= order.volume;
    return true;
  }

  else if (auto it = sell_orders_.find(order_id); it.has_value()) {
    auto order = it.value();

    if (!sell_orders_.remove(order_id)) return false;

    volumes_[{order.price_q4, SELL_SIDE}] -= order.volume;
    return true;
  }

  return false;
}

TopOfBook OrderBook::getTopOfBook() const {
  return {buy_orders_.top(), sell_orders_.top()};
}

std::optional<Order> OrderBook::getOrder(uint64_t order_id) const {
  if (auto it = buy_orders_.find(order_id); it.has_value()) {
    return it.value();
  }

  if (auto it = sell_orders_.find(order_id); it.has_value()) {
    return it.value();
  }

  return std::nullopt;
}

}  // namespace FastLittleMarket