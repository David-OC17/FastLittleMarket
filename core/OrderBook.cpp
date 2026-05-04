#include "OrderBook.hpp"

#include <cassert>

namespace fast_little_market {

bool OrderBook::canCross(const Order& incoming) const {
  const OrderQueueInterface* opposite =
      (incoming.side_ == BUY_SIDE)
          ? static_cast<const OrderQueueInterface*>(&sell_orders_)
          : static_cast<const OrderQueueInterface*>(&buy_orders_);

  if (opposite->empty() || !opposite->top().has_value()) return false;

  return priceCrosses(incoming, opposite->top().value());
}

bool OrderBook::priceCrosses(const Order& incoming,
                             const Order& opposite) const {
  return (incoming.side_ == BUY_SIDE)
             ? (opposite.price_q4_ <= incoming.price_q4_)
             : (opposite.price_q4_ >= incoming.price_q4_);
}

ExecFlags OrderBook::newOrder(const Order& order) {
  if (!order.isValid()) return ExecFlags::Rejected;

  auto flags = ExecFlags::None;
  if (canCross(order)) {
    flags = matchOrder(order);
    if (hasFlag(flags, ExecFlags::Rejected)) {
      return ExecFlags::Rejected;
    }
  } else {
    auto* own_queue = (order.side_ == BUY_SIDE)
                          ? static_cast<OrderQueueInterface*>(&buy_orders_)
                          : static_cast<OrderQueueInterface*>(&sell_orders_);

    assert(own_queue->isValid());

    own_queue->push(order);
    volumes_[std::make_pair(order.price_q4_, order.side_)] += order.volume_;
  }

  return flags | ExecFlags::Accepted;
}

ExecFlags OrderBook::matchOrder(Order incoming) {
  if (!incoming.isValid()) return ExecFlags::Rejected;

  auto* opposite_queue = (incoming.side_ == BUY_SIDE)
                             ? static_cast<OrderQueueInterface*>(&sell_orders_)
                             : static_cast<OrderQueueInterface*>(&buy_orders_);

  assert(opposite_queue->isValid());

  while (incoming.isValid() && !opposite_queue->empty()) {
    Order best_opposite = opposite_queue->top().value();

    if (!priceCrosses(incoming, best_opposite)) break;
    opposite_queue->pop();

    const int trade_volume = std::min(incoming.volume_, best_opposite.volume_);

    incoming.volume_ -= trade_volume;
    best_opposite.volume_ -= trade_volume;

    if (best_opposite.isValid()) {
      opposite_queue->push(best_opposite);
    }
  }

  if (incoming.volume_ <= 0) {
    return ExecFlags::FullyFilled;
  }

  auto* own_queue = (incoming.side_ == BUY_SIDE)
                        ? static_cast<OrderQueueInterface*>(&buy_orders_)
                        : static_cast<OrderQueueInterface*>(&sell_orders_);
  assert(own_queue->isValid());
  own_queue->push(std::move(incoming));

  return ExecFlags::PartiallyFilled;
}

ExecFlags OrderBook::cancelOrder(uint64_t order_id) {
  if (auto it = buy_orders_.find(order_id); it.has_value()) {
    auto order = it.value();

    if (!buy_orders_.remove(order_id)) return ExecFlags::Rejected;

    volumes_[{order.price_q4_, BUY_SIDE}] -= order.volume_;
    return ExecFlags::Cancelled;
  }

  else if (auto it = sell_orders_.find(order_id); it.has_value()) {
    auto order = it.value();

    if (!sell_orders_.remove(order_id)) return ExecFlags::Rejected;

    volumes_[{order.price_q4_, SELL_SIDE}] -= order.volume_;
    return ExecFlags::Cancelled;
  }

  return ExecFlags::Rejected;
}

ExecFlags OrderBook::modifyOrder(uint64_t order_id, uint32_t new_price_q4,
                                 uint32_t new_volume,
                                 GlobalSequencer& sequencer) {
  auto it = buy_orders_.find(order_id);
  if (!it.has_value()) return ExecFlags::Rejected;

  auto order = it.value();

  auto flags = cancelOrder(order_id);
  if (hasFlag(flags, ExecFlags::Rejected)) return flags;

  Order new_order(Order::unpack(order.id_ns_).id, new_price_q4, new_volume,
                  order.side_ == BUY_SIDE, order.client_, sequencer);

  return flags | newOrder(new_order);
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

}  // namespace fast_little_market