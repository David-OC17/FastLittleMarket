#pragma once

#include <atomic>
#include <chrono>

namespace FastLittleMarket {

struct EventId {
  uint64_t timestamp_shard;
  uint64_t epoch;

  bool operator<(const EventId& other) const {
    if (epoch != other.epoch) return epoch < other.epoch;
    return timestamp_shard < other.timestamp_shard;
  }
};

class GlobalSequencer {
 private:
  alignas(64) std::atomic<uint64_t> epoch_counter_{0};
  alignas(64) uint16_t shard_id_;

  static constexpr uint64_t EPOCH_MASK = (1ULL << 48) - 1;

 public:
  explicit GlobalSequencer(uint16_t shard) : shard_id_(shard) {}

  EventId next() {
    uint64_t epoch = epoch_counter_.fetch_add(1, std::memory_order_relaxed);
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                  std::chrono::steady_clock::now().time_since_epoch())
                  .count();

    return EventId{(static_cast<uint64_t>(shard_id_) << 48) | (ns & EPOCH_MASK),
                   epoch};
  }
};

}  // namespace FastLittleMarket