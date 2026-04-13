#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace FastLittleMarket {

static constexpr uint8_t BUY_SIDE = 1;
static constexpr uint8_t SELL_SIDE = 0;

static constexpr uint8_t MARKET_TYPE = 0;
static constexpr uint8_t LIMIT_TYPE = 1;

static constexpr uint8_t DAY_TIF = 0;
static constexpr uint8_t GTC_TIF = 1;
static constexpr uint8_t IOC_TIF = 2;

struct Order {        // 32 bytes
  uint64_t id_ns;     // Packed: ID << 32
  uint32_t price_q4;  // Price × 10000
  uint32_t volume;
  uint8_t side : 1;  // Buy=1/Sell=0
  uint8_t type : 2;  // Market=0/Limit=1
  uint8_t tim : 2;   // Day=0/GTC=1
  char client[8];    // "JPMORG\0"

  Order() = default;

  Order(int id, uint32_t price_q4, uint32_t vol, bool is_buy,
        std::string_view client)
      : id_ns(static_cast<uint64_t>(id) << 32),
        price_q4(price_q4),
        volume(vol),
        side(static_cast<uint8_t>(is_buy)) {
    std::strncpy(this->client, client.data(), 7);
    this->client[7] = '\0';
  }

  Order(const Order&) = default;
  Order(Order&&) = default;

  Order& operator=(const Order& other) noexcept {
    if (this != &other) {
      id_ns = other.id_ns;
      price_q4 = other.price_q4;
      volume = other.volume;
      side = other.side;
      type = other.type;
      tim = other.tim;
      std::memcpy(client, other.client, 8);
    }
    return *this;
  }

  Order& operator=(Order&& other) noexcept {
    id_ns = other.id_ns;
    price_q4 = other.price_q4;
    volume = other.volume;
    side = other.side;
    type = other.type;
    tim = other.tim;
    std::memcpy(client, other.client, 8);
    return *this;
  }

  bool operator==(const Order& other) const noexcept {
    return id_ns == other.id_ns;
  }

  bool operator!=(const Order& other) const = default;

  bool operator<(const Order& other) const noexcept {
    if (price_q4 != other.price_q4) return price_q4 < other.price_q4;
    return id_ns < other.id_ns;  // FIFO
  }

  [[nodiscard]] bool isValid() const noexcept {
    if (volume == 0) return false;        // No zero volume
    if (price_q4 == 0) return false;      // No zero price
    if (client[0] == '\0') return false;  // Client required

    return true;
  }
};

}  // namespace FastLittleMarket