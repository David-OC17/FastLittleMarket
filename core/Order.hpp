#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "GlobalSequencer.hpp"

namespace fast_little_market {

static constexpr uint8_t BUY_SIDE = 1;
static constexpr uint8_t SELL_SIDE = 0;

static constexpr uint8_t MARKET_TYPE = 0;
static constexpr uint8_t LIMIT_TYPE = 1;

static constexpr uint8_t DAY_TIF = 0;
static constexpr uint8_t GTC_TIF = 1;
static constexpr uint8_t IOC_TIF = 2;

struct IdTimestamp {
  uint32_t id;
  uint32_t timestamp_ns;
};

struct Order {         // 32 bytes
  uint64_t id_ns_;     // Packed: ID << 32 | Timestamp
  uint32_t price_q4_;  // Price × 10000
  uint32_t volume_;
  uint8_t side_ : 1;  // Buy=1/Sell=0
  uint8_t type_ : 2;  // Market=0/Limit=1
  uint8_t tim_ : 2;   // Day=0/GTC=1
  char client_[8];    // "JPMORG\0"

  static inline uint64_t pack(uint32_t id, uint32_t ts_ns) {
    return (uint64_t(id) << 32) | ts_ns;
  }

  static inline IdTimestamp unpack(uint64_t id_ns_) {
    return {static_cast<uint32_t>(id_ns_ >> 32),
            static_cast<uint32_t>(id_ns_ & 0xFFFFFFFFULL)};
  }

  Order() = default;

  Order(int id, uint32_t price_q4_, uint32_t vol, bool is_buy,
        std::string_view client_, GlobalSequencer& sequencer)
      : id_ns_(pack(id, sequencer.nextTimestampNs())),
        price_q4_(price_q4_),
        volume_(vol),
        side_(static_cast<uint8_t>(is_buy)) {
    std::strncpy(this->client_, client_.data(), 7);
    this->client_[7] = '\0';
  }

  Order(const Order&) = default;
  Order(Order&&) = default;

  Order& operator=(const Order& other) noexcept {
    if (this != &other) {
      id_ns_ = other.id_ns_;
      price_q4_ = other.price_q4_;
      volume_ = other.volume_;
      side_ = other.side_;
      type_ = other.type_;
      tim_ = other.tim_;
      std::memcpy(client_, other.client_, 8);
    }
    return *this;
  }

  Order& operator=(Order&& other) noexcept {
    id_ns_ = other.id_ns_;
    price_q4_ = other.price_q4_;
    volume_ = other.volume_;
    side_ = other.side_;
    type_ = other.type_;
    tim_ = other.tim_;
    std::memcpy(client_, other.client_, 8);
    return *this;
  }

  bool operator==(const Order& other) const noexcept {
    return id_ns_ == other.id_ns_;
  }

  bool operator!=(const Order& other) const = default;

  bool operator<(const Order& other) const noexcept {
    if (price_q4_ != other.price_q4_) return price_q4_ < other.price_q4_;
    return id_ns_ < other.id_ns_;  // FIFO
  }

  [[nodiscard]] bool isValid() const noexcept {
    if (volume_ == 0) return false;        // No zero volume_
    if (price_q4_ == 0) return false;      // No zero price
    if (client_[0] == '\0') return false;  // Client required

    return true;
  }
};

}  // namespace fast_little_market