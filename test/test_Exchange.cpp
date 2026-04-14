#include <gtest/gtest.h>

#include <thread>

#include "GlobalSequencer.hpp"
#include "Exchange.hpp"

namespace flm = FastLittleMarket;

// Constructor: Order(int id, uint32_t price_q4, uint32_t vol, bool is_buy,
//                   std::string_view client)
// addOrder / cancelOrder / getOrder all use plain integer order ids, not id_ns.

class ExchangeTest : public ::testing::Test {
 protected:
  flm::GlobalSequencer sequencer_;
};

TEST_F(ExchangeTest, Singleton) {
  flm::Exchange& exchange1 = flm::Exchange::getInstance();
  flm::Exchange& exchange2 = flm::Exchange::getInstance();
  EXPECT_EQ(&exchange1, &exchange2);
}

TEST_F(ExchangeTest, GetShard) {
  flm::Exchange& exchange = flm::Exchange::getInstance();

  EXPECT_EQ(exchange.getShard("AAPL"),
            std::hash<std::string>{}("AAPL") % flm::NUM_SHARDS);
  EXPECT_EQ(exchange.getShard("GOOG"),
            std::hash<std::string>{}("GOOG") % flm::NUM_SHARDS);
  EXPECT_EQ(exchange.getShard("MSFT"),
            std::hash<std::string>{}("MSFT") % flm::NUM_SHARDS);
}

TEST_F(ExchangeTest, AddAndCancelOrderSingleThread) {
  flm::Exchange& exchange = flm::Exchange::getInstance();
  std::string symbol = "AAPL";

  // Constructor: (id, price_q4, vol, is_buy, client)
  // $150.00 = 1500000 in price_q4 (× 10000)
  flm::Order order(1, 1500000, 100, flm::BUY_SIDE, "ClientA", sequencer_);
  flm::Order order_copy = order;

  const uint64_t orderId = order.id_ns;

  exchange.addOrder(symbol, order);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  EXPECT_TRUE(exchange.getOrder(symbol, orderId).has_value());
  EXPECT_EQ(exchange.getOrder(symbol, orderId).value(), order_copy);

  exchange.cancelOrder(symbol, orderId);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  EXPECT_FALSE(exchange.getOrder(symbol, orderId).has_value());
}

TEST_F(ExchangeTest, ParallelSymbolProcessing) {
  auto& exchange = flm::Exchange::getInstance();

  // Constructor: (id, price_q4, vol, is_buy, client)
  // Symbols AAPL and TSLA are likely on different shards, so these threads
  // exercise independent shard workers in parallel.
  std::thread t1([&exchange, this]() {
    for (int i = 0; i < 1000; ++i)
      exchange.addOrder("AAPL",
                        flm::Order(i, 1000000 + i * 10, 10, flm::BUY_SIDE, "t1", sequencer_));
  });

  std::thread t2([&exchange, this]() {
    for (int i = 0; i < 1000; ++i)
      exchange.addOrder("TSLA",
                        flm::Order(i + 1000, 2000000 + i * 10, 10, flm::BUY_SIDE, "t2", sequencer_));
  });

  // GOOG: interleaved add+cancel on the same symbol exercises shard ordering
  std::thread t3([&exchange, this]() {
    for (int i = 0; i < 1000; ++i) {
      exchange.addOrder("GOOG",
                        flm::Order(i + 2000, 1000000 + i * 10, 10, flm::BUY_SIDE, "t3", sequencer_));
      exchange.cancelOrder("GOOG", i + 2000);
    }
  });

  t1.join();
  t2.join();
  t3.join();

  // No assertion on final state — this test validates that no crash, deadlock,
  // or data race occurs under concurrent access. Run under ThreadSanitizer
  // (TSAN) to catch races that the sleep-based tests would miss.
}