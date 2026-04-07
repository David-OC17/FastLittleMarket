#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = FastLittleMarket;

TEST(OrderBook, AddAndTopOfBook) {
  flm::OrderBook ob;

  flm::Order order1(1, flm::OrderSide::Buy, 100.0, 10, "client1");
  flm::Order order2(2, flm::OrderSide::Sell, 101.0, 5, "client2");

  ob.addOrder(order1);
  ob.addOrder(order2);

  auto top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_TRUE(top.hasAsk());
  EXPECT_EQ(top.bid->getId(), order1.getId());
  EXPECT_EQ(top.ask->getId(), order2.getId());
}

TEST(OrderBook, CancelOrder) {
  flm::OrderBook ob;

  flm::Order order1(1, flm::OrderSide::Buy, 100.0, 10, "client1");
  flm::Order order2(2, flm::OrderSide::Sell, 101.0, 5, "client2");
  flm::Order order3(3, flm::OrderSide::Buy, 99.0, 15, "client3");

  ob.addOrder(order1);
  ob.addOrder(order2);

  auto top = ob.getTopOfBook();

  EXPECT_TRUE(ob.cancelOrder(order1.getId()));
  top = ob.getTopOfBook();
  EXPECT_FALSE(top.hasBid());

  EXPECT_TRUE(ob.cancelOrder(order2.getId()));
  EXPECT_FALSE(ob.cancelOrder(999));  // Non-existent order

  ob.addOrder(order3);

  top = ob.getTopOfBook();

  EXPECT_TRUE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
  EXPECT_EQ(top.bid->getId(), order3.getId());
}

TEST(OrderBook, MatchOrders) {
  flm::OrderBook ob;

  flm::Order bid_order(1, flm::OrderSide::Buy, 100.0, 10, "client1");
  flm::Order ask_order(2, flm::OrderSide::Sell, 99.0, 5, "client2");

  ob.addOrder(bid_order);
  ob.addOrder(ask_order);

  auto top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
  EXPECT_EQ(top.bid->getVolume(), 5);  // bid order partially filled
  EXPECT_FALSE(top.ask.has_value());   // ask order fully filled
}