#pragma once

#include <chrono>
#include <string>

namespace FastLittleMarket {

enum class OrderSide { Buy, Sell };

class Order {
 private:
  int order_id_;
  std::chrono::system_clock::time_point timestamp_;

  OrderSide side_;
  double price_;
  int volume_;
  std::string client_;

 public:
  Order(int order_id, OrderSide side, double price, int volume,
        std::string client);

  Order(const Order& other);

  Order& operator=(const Order& other);

  bool isValid() const;

  // Getters
  int getId() const;
  std::chrono::system_clock::time_point getTimestamp() const;
  OrderSide getSide() const;
  double getPrice() const;
  int getVolume() const;
  std::string getClient() const;

  // Setters
  void setPrice(double price);
  void setVolume(int volume);
  void setClient(std::string client);
  void setSide(OrderSide side);
};

}  // namespace FastLittleMarket