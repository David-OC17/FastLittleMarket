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

  bool isValid() const;

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

class OrderQueueInterface {
 public:
  virtual ~OrderQueueInterface() = default;
  virtual bool empty() const = 0;
  virtual std::optional<Order> top() const = 0;
  virtual void push(const Order& order) = 0;
  virtual void pop() = 0;
  virtual bool remove(int order_id) = 0;
  virtual bool modify(int order_id, const Order& order) = 0;
  virtual bool isValid() const = 0;
};

}  // namespace FastLittleMarket