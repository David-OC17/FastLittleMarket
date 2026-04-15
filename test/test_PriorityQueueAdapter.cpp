#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = FastLittleMarket;

using PQ = flm::PriorityQueueAdapter<flm::BuyOrderComparator>;

class PriorityQueuesAdapterTest : public ::testing::Test {
 protected:
  flm::GlobalSequencer sequencer_;
};

TEST_F(PriorityQueuesAdapterTest, PopReturnsFalseWhenEmpty) {
  PQ pq;

  EXPECT_FALSE(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST_F(PriorityQueuesAdapterTest, PopReturnsTrueWhenNotEmpty) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));

  EXPECT_TRUE(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST_F(PriorityQueuesAdapterTest, MultiplePopsMaintainConsistency) {
  PQ pq;

  // BuyOrderComparator: highest price_q4 first; ties broken by lowest id_ns
  pq.push(
      flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));  // $100.00
  pq.push(flm::Order(2, 1100000, 10, flm::BUY_SIDE, "c2",
                     sequencer_));  // $110.00 — top
  pq.push(
      flm::Order(3, 1050000, 10, flm::BUY_SIDE, "c3", sequencer_));  // $105.00

  EXPECT_TRUE(pq.pop());  // removes id=2 ($110.00)
  EXPECT_TRUE(pq.pop());  // removes id=3 ($105.00)
  EXPECT_TRUE(pq.pop());  // removes id=1 ($100.00)

  EXPECT_TRUE(pq.empty());
  EXPECT_FALSE(pq.pop());  // now empty
}

TEST_F(PriorityQueuesAdapterTest, EmptyInitially) {
  PQ pq;
  EXPECT_TRUE(pq.empty());
  EXPECT_FALSE(pq.top().has_value());
}

TEST_F(PriorityQueuesAdapterTest, PushAndTop) {
  PQ pq;

  pq.push(
      flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));  // $100.00
  pq.push(flm::Order(2, 1050000, 10, flm::BUY_SIDE, "c2",
                     sequencer_));  // $105.00 — higher, goes to top

  ASSERT_TRUE(pq.top().has_value());
  // BuyOrderComparator puts highest price first; id=2 has the higher price
  EXPECT_EQ(flm::Order::unpack(pq.top()->id_ns).id, static_cast<uint32_t>(2));
}

TEST_F(PriorityQueuesAdapterTest, PushInvalidOrderIgnored) {
  PQ pq;

  // volume=0 makes isValid() return false; push() rejects invalid orders
  pq.push(flm::Order(1, 1000000, 0, flm::BUY_SIDE, "c1", sequencer_));
  EXPECT_TRUE(pq.empty());
}

TEST_F(PriorityQueuesAdapterTest, FindExistingAndMissing) {
  PQ pq;

  flm::Order order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_);
  pq.push(order);

  const uint64_t order_id_ns = order.id_ns;

  // find() takes the raw int order_id, not id_ns
  auto found = pq.find(order_id_ns);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(flm::Order::unpack(found->id_ns).id, static_cast<uint32_t>(1));

  EXPECT_FALSE(pq.find(999).has_value());
}

TEST_F(PriorityQueuesAdapterTest, PopRemovesTop) {
  PQ pq;

  pq.push(
      flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));  // $100.00
  pq.push(flm::Order(2, 1050000, 10, flm::BUY_SIDE, "c2",
                     sequencer_));  // $105.00 — top

  ASSERT_EQ(flm::Order::unpack(pq.top()->id_ns).id, static_cast<uint32_t>(2));

  pq.pop();

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(flm::Order::unpack(pq.top()->id_ns).id, static_cast<uint32_t>(1));
}

TEST_F(PriorityQueuesAdapterTest, PopOnEmptyDoesNothing) {
  PQ pq;

  EXPECT_NO_THROW(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST_F(PriorityQueuesAdapterTest, RemoveExistingOrder) {
  PQ pq;

  flm::Order order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_);
  pq.push(order);

  const uint64_t order_id_ns = order.id_ns;

  EXPECT_TRUE(pq.remove(order_id_ns));
  EXPECT_TRUE(pq.empty());
}

TEST_F(PriorityQueuesAdapterTest, RemoveNonExistingOrder) {
  PQ pq;

  EXPECT_FALSE(pq.remove(42));
}

TEST_F(PriorityQueuesAdapterTest, RemoveUpdatesTopCorrectly) {
  PQ pq;

  flm::Order order2(2, 1100000, 10, flm::BUY_SIDE, "c2", sequencer_);

  pq.push(
      flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));  // $100.00
  pq.push(order2);  // $110.00 — top

  const uint64_t order2_id_ns = order2.id_ns;

  EXPECT_TRUE(pq.remove(order2_id_ns));

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(flm::Order::unpack(pq.top()->id_ns).id, static_cast<uint32_t>(1));
}