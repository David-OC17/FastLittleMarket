#include "Order.hpp"

namespace FastLittleMarket {

Order::Order(int order_id, BuyOrSell side, double price, int volume,
             std::string client)
    : order_id_(order_id),
      timestamp_(std::chrono::system_clock::now()),
      side_(side),
      price_(price),
      volume_(volume),
      client_(client) {}

Order::Order(const Order& other)
    : order_id_(other.order_id_),
      timestamp_(other.timestamp_),
      side_(other.side_),
      price_(other.price_),
      volume_(other.volume_),
      client_(other.client_) {}

Order& Order::operator=(const Order& other) {
  if (this != &other) {
    order_id_ = other.order_id_;
    timestamp_ = other.timestamp_;
    side_ = other.side_;
    price_ = other.price_;
    volume_ = other.volume_;
    client_ = other.client_;
  }
  return *this;
}

bool Order::isValid() const {
  return order_id_ >= 0 && volume_ > 0 && price_ > 0.0 &&
         (side_ == BuyOrSell::buy || side_ == BuyOrSell::sell) &&
         !client_.empty();
}

int Order::getId() const { return order_id_; }
std::chrono::system_clock::time_point Order::getTimestamp() const {
  return timestamp_;
}
BuyOrSell Order::getSide() const { return side_; }
double Order::getPrice() const { return price_; }
int Order::getVolume() const { return volume_; }
std::string Order::getClient() const { return client_; }

void Order::setPrice(double price) { price_ = price; }
void Order::setVolume(int volume) { volume_ = volume; }
void Order::setClient(std::string client) { client_ = client; }
void Order::setSide(BuyOrSell side) { side_ = side; }

}  // namespace FastLittleMarket