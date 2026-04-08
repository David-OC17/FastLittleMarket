#include <gtest/gtest.h>

#include <thread>

#include "Exchange.hpp"

namespace flm = FastLittleMarket;

/*
3 pillars: correctness, race detection, performance

UNIT TESTS

Exchange {
Under one ThreadPool worker:
- addOrder() works: verify (directly on OrderBook) order has been added
- cancelOrder() works: verify (directly on OrderBook) order has been canceled
Under multiple ThreadPool workers:
- addOrder() works: verify order, verify parallelism (2 tests)
- cancelOrder() works: verify order, verify parallelism (2 tests)
}
*/

TEST(ExchangeTest, Singleton) {
  flm::Exchange& exchange1 = flm::Exchange::getInstance();
  flm::Exchange& exchange2 = flm::Exchange::getInstance();
  EXPECT_EQ(&exchange1, &exchange2);
}

TEST(ExchangeTest, GetShard) {
  flm::Exchange& exchange = flm::Exchange::getInstance();

  EXPECT_EQ(exchange.getShard("AAPL"),
            std::hash<std::string>{}("AAPL") % flm::NUM_SHARDS);
  EXPECT_EQ(exchange.getShard("GOOG"),
            std::hash<std::string>{}("GOOG") % flm::NUM_SHARDS);
  EXPECT_EQ(exchange.getShard("MSFT"),
            std::hash<std::string>{}("MSFT") % flm::NUM_SHARDS);
}

TEST(ExchangeTest, AddAndCancelOrderSingleThread) {
  flm::Exchange& exchange = flm::Exchange::getInstance();
  std::string symbol = "AAPL";
  int orderId = 1;
  flm::Order order{orderId, flm::OrderSide::Buy, 150.0, 100, "ClientA"};
  flm::Order order_copy = order;

  exchange.addOrder(symbol, order);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  EXPECT_TRUE(exchange.getOrder(symbol, orderId).has_value());
  EXPECT_EQ(exchange.getOrder(symbol, orderId).value(), order_copy);

  exchange.cancelOrder(symbol, orderId);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  EXPECT_TRUE(!exchange.getOrder(symbol, orderId).has_value());
}

TEST(ExchangeTest, ParallelSymbolProcessing) {
  auto& exchange = flm::Exchange::getInstance();

  std::thread t1([&exchange]() {
    for (int i = 0; i < 1000; ++i)
      exchange.addOrder(
          "AAPL", flm::Order(i, flm::OrderSide::Buy, 100 + i * 0.01, 10, "t1"));
  });

  std::thread t2([&exchange]() {
    for (int i = 0; i < 1000; ++i)
      exchange.addOrder("TSLA", flm::Order(i + 1000, flm::OrderSide::Buy,
                                           200 + i * 0.01, 10, "t2"));
  });

  std::thread t3([&exchange]() {
    for (int i = 0; i < 1000; ++i) {
      exchange.addOrder(
          "GOOG", flm::Order(i, flm::OrderSide::Buy, 100 + i * 0.01, 10, "t1"));
      exchange.cancelOrder("GOOG", i);
    }
  });

  t1.join();
  t2.join();
  t3.join();
}