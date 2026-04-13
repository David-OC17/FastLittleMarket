#include <atomic>
#include <chrono>

#include "Order.hpp"
#include "quill/Logger.h"
#include "quill/SimpleSetup.h"

namespace FastLittleMarket {

class ExchangeLogger {
 public:
  void logEvent(EventId seq_id, EventType type, const std::string& symbol,
                const Order& order) {
    LOG_INFO(
        "EVENT|seq={:020}|ts={}|shard={}|type={} sym={} id={} side={} p={} "
        "v={}",
        seq_id.epoch, (seq_id.timestamp_shard >> 16) & 0xFFFFFFFFFFFFULL,
        seq_id.timestamp_shard >> 48, EventTypeNames[static_cast<size_t>(type)],
        symbol, order.id_ns(), order.getSide(), order.getPrice(),
        order.volume());
  };

  void logError(const std::string& message) {
    // TODO: consider adding more context to error logs, such as timestamps,
    // shard info, etc.
    LOG_ERROR("ERROR|{}", message);
  }
}

}  // namespace FastLittleMarket