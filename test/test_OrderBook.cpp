#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = FastLittleMarket;

// Constructor: Order(int id, uint32_t price_q4, uint32_t vol, bool is_buy,
//                   std::string_view client)
// id_ns = static_cast<uint64_t>(id) << 32
//
// cancelOrder(int order_id) and getOrder(int order_id) take the plain integer
// id, NOT id_ns. To recover it from an Order: order.id_ns >> 32.
//
// price_q4 is price × 10000 — bid at $100.00 = 1000000, ask at $101.00 =
// 1010000. A bid/ask cross occurs when bid price_q4 >= ask price_q4.

TEST(OrderBookTest, AddAndTopOfBook) {
  flm::OrderBook ob;

  // bid $100.00, ask $101.00 — no cross, both rest on the book
  flm::Order order1(1, 1000000, 10, flm::BUY_SIDE, "client1");
  flm::Order order2(2, 1010000, 5, flm::SELL_SIDE, "client2");

  EXPECT_TRUE(ob.addOrder(order1));
  EXPECT_TRUE(ob.addOrder(order2));

  auto top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_TRUE(top.hasAsk());
  EXPECT_EQ(top.bid->id_ns, order1.id_ns);
  EXPECT_EQ(top.ask->id_ns, order2.id_ns);
}

TEST(OrderBookTest, CancelOrder) {
  flm::OrderBook ob;

  flm::Order order1(1, 1000000, 10, flm::BUY_SIDE, "client1");
  flm::Order order2(2, 1010000, 5, flm::SELL_SIDE, "client2");
  flm::Order order3(3, 990000, 15, flm::BUY_SIDE, "client3");

  ob.addOrder(order1);
  ob.addOrder(order2);

  EXPECT_TRUE(ob.cancelOrder(1));
  auto top = ob.getTopOfBook();
  EXPECT_FALSE(top.hasBid());

  EXPECT_TRUE(ob.cancelOrder(2));
  EXPECT_FALSE(ob.cancelOrder(999));  // non-existent order

  ob.addOrder(order3);

  top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
  EXPECT_EQ(top.bid->id_ns, order3.id_ns);
}

TEST(OrderBookTest, MatchOrders) {
  flm::OrderBook ob;

  // bid at $100.00, ask at $99.00 — prices cross, matching occurs
  flm::Order bid_order(1, 1000000, 10, flm::BUY_SIDE, "client1");
  flm::Order ask_order(2, 990000, 5, flm::SELL_SIDE, "client2");

  EXPECT_TRUE(ob.addOrder(bid_order));
  EXPECT_TRUE(ob.addOrder(ask_order));

  auto top = ob.getTopOfBook();
  // ask fully consumed (vol 5 <= bid vol 10); bid partially filled, 5 remain
  EXPECT_TRUE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
  EXPECT_EQ(top.bid->volume, 5u);
}

TEST(OrderBookTest, FullMatchClearsBothSides) {
  flm::OrderBook ob;

  // Equal volumes — both sides fully consumed
  flm::Order bid_order(1, 1000000, 10, flm::BUY_SIDE, "client1");
  flm::Order ask_order(2, 990000, 10, flm::SELL_SIDE, "client2");

  ob.addOrder(bid_order);
  ob.addOrder(ask_order);

  auto top = ob.getTopOfBook();
  EXPECT_FALSE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
}

TEST(OrderBookTest, AddAndGetOrder) {
  flm::OrderBook ob;

  // No cross: bid $100.00, ask $101.00
  flm::Order order1(1, 1000000, 10, flm::BUY_SIDE, "client1");
  flm::Order order2(2, 1010000, 5, flm::SELL_SIDE, "client2");

  ob.addOrder(order1);
  ob.addOrder(order2);

  // getOrder takes the plain integer id
  auto retrieved_order1 = ob.getOrder(1);
  auto retrieved_order2 = ob.getOrder(2);

  EXPECT_TRUE(retrieved_order1.has_value());
  EXPECT_TRUE(retrieved_order2.has_value());
  // operator== compares id_ns only
  EXPECT_EQ(retrieved_order1.value(), order1);
  EXPECT_EQ(retrieved_order2.value(), order2);
}

TEST(OrderBookTest, GetNonExistentOrder) {
  flm::OrderBook ob;

  EXPECT_FALSE(ob.getOrder(999).has_value());
}