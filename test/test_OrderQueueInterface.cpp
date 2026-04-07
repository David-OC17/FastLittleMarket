#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = FastLittleMarket;

using PQ = flm::PriorityQueueAdapter<flm::BuyOrderComparator>;
// using SellOrderQueue = flm::PriorityQueueAdapter<flm::SellOrderComparator>;

TEST(PriorityQueueAdapter, PopReturnsFalseWhenEmpty) {
  PQ pq;

  EXPECT_FALSE(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapter, PopReturnsTrueWhenNotEmpty) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));

  EXPECT_TRUE(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapter, MultiplePopsMaintainConsistency) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));
  pq.push(flm::Order(2, flm::OrderSide::Buy, 110.0, 10, "c2"));
  pq.push(flm::Order(3, flm::OrderSide::Buy, 105.0, 10, "c3"));

  EXPECT_TRUE(pq.pop());  // removes 2
  EXPECT_TRUE(pq.pop());  // removes 3
  EXPECT_TRUE(pq.pop());  // removes 1

  EXPECT_TRUE(pq.empty());
  EXPECT_FALSE(pq.pop());  // now empty
}

TEST(PriorityQueueAdapter, EmptyInitially) {
  PQ pq;
  EXPECT_TRUE(pq.empty());
  EXPECT_FALSE(pq.top().has_value());
}

TEST(PriorityQueueAdapter, PushAndTop) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));
  pq.push(flm::Order(2, flm::OrderSide::Buy, 105.0, 10, "c2"));

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(pq.top()->getId(), 2);  // highest price first
}

TEST(PriorityQueueAdapter, PushInvalidOrderIgnored) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, -10, "c1"));  // invalid
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapter, FindExistingAndMissing) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));

  auto found = pq.find(1);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->getId(), 1);

  EXPECT_FALSE(pq.find(999).has_value());
}

TEST(PriorityQueueAdapter, PopRemovesTop) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));
  pq.push(flm::Order(2, flm::OrderSide::Buy, 105.0, 10, "c2"));

  ASSERT_EQ(pq.top()->getId(), 2);
  pq.pop();

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(pq.top()->getId(), 1);
}

TEST(PriorityQueueAdapter, PopOnEmptyDoesNothing) {
  PQ pq;

  EXPECT_NO_THROW(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapter, RemoveExistingOrder) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));

  EXPECT_TRUE(pq.remove(1));
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapter, RemoveNonExistingOrder) {
  PQ pq;

  EXPECT_FALSE(pq.remove(42));
}

TEST(PriorityQueueAdapter, RemoveUpdatesTopCorrectly) {
  PQ pq;

  pq.push(flm::Order(1, flm::OrderSide::Buy, 100.0, 10, "c1"));
  pq.push(flm::Order(2, flm::OrderSide::Buy, 110.0, 10, "c2"));

  EXPECT_TRUE(pq.remove(2));

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(pq.top()->getId(), 1);
}