#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = fast_little_market;

class OrderBookTest : public ::testing::Test {
 protected:
  flm::GlobalSequencer sequencer_;
};

TEST_F(OrderBookTest, AddAndTopOfBook) {
  flm::OrderBook ob;

  // bid $100.00, ask $101.00 — no cross, both rest on the book
  flm::Order order1(1, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order order2(2, 1010000, 5, flm::SELL_SIDE, "client2", sequencer_);

  EXPECT_EQ(ob.newOrder(order1), flm::ExecFlags::Accepted);
  EXPECT_EQ(ob.newOrder(order2), flm::ExecFlags::Accepted);

  auto top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_TRUE(top.hasAsk());
  EXPECT_EQ(top.bid->id_ns, order1.id_ns);
  EXPECT_EQ(top.ask->id_ns, order2.id_ns);
}

TEST_F(OrderBookTest, CancelOrder) {
  flm::OrderBook ob;

  flm::Order order1(33, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order order2(38, 1010000, 5, flm::SELL_SIDE, "client2", sequencer_);
  flm::Order order3(45, 990000, 15, flm::BUY_SIDE, "client3", sequencer_);

  ob.newOrder(order1);
  ob.newOrder(order2);  // Expect no match with order1, as $100.00 < $101.00

  const uint64_t order1_id_ns = order1.id_ns;
  const uint64_t order2_id_ns = order2.id_ns;
  const uint64_t order3_id_ns = order3.id_ns;

  EXPECT_EQ(ob.cancelOrder(order1_id_ns), flm::ExecFlags::Cancelled);
  auto top = ob.getTopOfBook();
  EXPECT_FALSE(top.hasBid());

  EXPECT_EQ(ob.cancelOrder(order2_id_ns), flm::ExecFlags::Cancelled);
  EXPECT_EQ(ob.cancelOrder(999),
            flm::ExecFlags::Rejected);  // non-existent order

  ob.newOrder(order3);

  top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
  EXPECT_EQ(top.bid->id_ns, order3_id_ns);
}

TEST_F(OrderBookTest, ModifyOrder) {
  flm::OrderBook ob;

  // ── Basic modify: engine cancels old order and reinserts — returns
  // Accepted|Cancelled
  flm::Order order1(1, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order order2(2, 1010000, 5, flm::SELL_SIDE, "client2", sequencer_);
  ob.newOrder(order1);
  ob.newOrder(order2);
  const uint64_t order1_id_ns = order1.id_ns;

  EXPECT_EQ(ob.modifyOrder(order1_id_ns, 990000, 7, sequencer_),
            flm::ExecFlags::Accepted | flm::ExecFlags::Cancelled);
  auto top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_EQ(top.bid->price_q4, 990000u);
  EXPECT_EQ(top.bid->volume, 7u);

  // ── Modify into a cross is rejected — engine does not match on modify ──────
  EXPECT_EQ(ob.modifyOrder(order1_id_ns, 1020000, 5, sequencer_),
            flm::ExecFlags::Rejected);
  // Order should still be live at its last accepted price
  top = ob.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_EQ(top.bid->price_q4, 990000u);

  // ── Modify a non-existent order is rejected ───────────────────────────────
  EXPECT_EQ(ob.modifyOrder(999, 1000000, 10, sequencer_),
            flm::ExecFlags::Rejected);

  // ── Priority displacement: modified order loses time priority ─────────────
  flm::OrderBook ob2;
  flm::Order order3(10, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order order4(11, 1050000, 8, flm::SELL_SIDE, "client2", sequencer_);
  flm::Order order5(12, 980000, 3, flm::BUY_SIDE, "client3", sequencer_);
  ob2.newOrder(order3);
  ob2.newOrder(order4);
  ob2.newOrder(order5);
  const uint64_t order3_id_ns = order3.id_ns;
  const uint64_t order4_id_ns = order4.id_ns;

  EXPECT_EQ(ob2.modifyOrder(order3_id_ns, 970000, 10, sequencer_),
            flm::ExecFlags::Accepted | flm::ExecFlags::Cancelled);
  top = ob2.getTopOfBook();
  EXPECT_TRUE(top.hasBid());
  EXPECT_NE(top.bid->id_ns, order3_id_ns);  // order3 no longer best bid
  EXPECT_EQ(top.ask->id_ns, order4_id_ns);  // ask unchanged

  // ── Modifying a cancelled order is rejected ───────────────────────────────
  flm::OrderBook ob3;
  flm::Order order6(20, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  ob3.newOrder(order6);
  const uint64_t order6_id_ns = order6.id_ns;
  ob3.cancelOrder(order6_id_ns);
  EXPECT_EQ(ob3.modifyOrder(order6_id_ns, 1010000, 5, sequencer_),
            flm::ExecFlags::Rejected);

  // ── Zero volume modify is rejected ────────────────────────────────────────
  flm::OrderBook ob4;
  flm::Order order7(30, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  ob4.newOrder(order7);
  EXPECT_TRUE(
      flm::hasFlag(ob4.modifyOrder(order7.id_ns, 1000000, 0, sequencer_),
                   flm::ExecFlags::Rejected));
}

TEST_F(OrderBookTest, MatchOrders) {
  flm::OrderBook ob;

  // bid at $100.00, ask at $99.00 — prices cross, matching occurs
  flm::Order bid_order(1, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order ask_order(2, 990000, 5, flm::SELL_SIDE, "client2", sequencer_);

  EXPECT_EQ(ob.newOrder(bid_order), flm::ExecFlags::Accepted);

  flm::ExecFlags ask_flags =
      flm::ExecFlags::Accepted | flm::ExecFlags::FullyFilled;
  EXPECT_EQ(ob.newOrder(ask_order), ask_flags);

  auto top = ob.getTopOfBook();
  // ask fully consumed (vol 5 <= bid vol 10); bid partially filled, 5 remain
  EXPECT_TRUE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
  EXPECT_EQ(top.bid->volume, 5u);
}

TEST_F(OrderBookTest, FullMatchClearsBothSides) {
  flm::OrderBook ob;

  // Equal volumes — both sides fully consumed
  flm::Order bid_order(1, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order ask_order(2, 990000, 10, flm::SELL_SIDE, "client2", sequencer_);

  ob.newOrder(bid_order);
  ob.newOrder(ask_order);

  auto top = ob.getTopOfBook();
  EXPECT_FALSE(top.hasBid());
  EXPECT_FALSE(top.hasAsk());
}

TEST_F(OrderBookTest, AddAndGetOrder) {
  flm::OrderBook ob;

  // No cross: bid $100.00, ask $101.00
  flm::Order order1(1, 1000000, 10, flm::BUY_SIDE, "client1", sequencer_);
  flm::Order order2(2, 1010000, 5, flm::SELL_SIDE, "client2", sequencer_);

  ob.newOrder(order1);
  ob.newOrder(order2);

  const uint64_t order1_id_ns = order1.id_ns;
  const uint64_t order2_id_ns = order2.id_ns;

  // getOrder takes the plain integer id
  auto retrieved_order1 = ob.getOrder(order1_id_ns);
  auto retrieved_order2 = ob.getOrder(order2_id_ns);

  EXPECT_TRUE(retrieved_order1.has_value());
  EXPECT_TRUE(retrieved_order2.has_value());
  // operator== compares id_ns only
  EXPECT_EQ(retrieved_order1.value(), order1);
  EXPECT_EQ(retrieved_order2.value(), order2);
}

TEST_F(OrderBookTest, GetNonExistentOrder) {
  flm::OrderBook ob;

  EXPECT_FALSE(ob.getOrder(999).has_value());
}