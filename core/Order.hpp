#pragma once

#include <chrono>
#include <string>

namespace FastLittleMarket {

enum BuyOrSell { buy, sell };

class Order {
 private:
  int order_id_;
  std::chrono::system_clock::time_point timestamp_;

  BuyOrSell side_;
  double price_;
  int volume_;
  std::string client_;

 public:
  Order(int order_id, BuyOrSell side, double price, int volume,
        std::string client);

  Order(const Order& other);

  Order& operator=(const Order& other);

  bool Order::isValid() const;

  // Getters
  int getId() const;
  std::chrono::system_clock::time_point getTimestamp() const;
  BuyOrSell getSide() const;
  double getPrice() const;
  int getVolume() const;
  std::string getClient() const;

  // Setters
  void setPrice(double price);
  void setVolume(int volume);
  void setClient(std::string client);
  void setSide(BuyOrSell side);
};

}  // namespace FastLittleMarket