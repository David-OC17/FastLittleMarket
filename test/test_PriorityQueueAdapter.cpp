#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = FastLittleMarket;

// Constructor: Order(int id, uint32_t price_q4, uint32_t vol, bool is_buy,
//                   std::string_view client)
// id_ns = static_cast<uint64_t>(id) << 32
// price_q4 is uint32_t — pass integer prices (e.g. 1000000 = $100.00 at ×10000)
// volume is uint32_t — passing 0 triggers isValid() == false, not negatives

using PQ = flm::PriorityQueueAdapter<flm::BuyOrderComparator>;

TEST(PriorityQueueAdapterTest, PopReturnsFalseWhenEmpty) {
  PQ pq;

  EXPECT_FALSE(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapterTest, PopReturnsTrueWhenNotEmpty) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));

  EXPECT_TRUE(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapterTest, MultiplePopsMaintainConsistency) {
  PQ pq;

  // BuyOrderComparator: highest price_q4 first; ties broken by lowest id_ns
  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));  // $100.00
  pq.push(flm::Order(2, 1100000, 10, flm::BUY_SIDE, "c2"));  // $110.00 — top
  pq.push(flm::Order(3, 1050000, 10, flm::BUY_SIDE, "c3"));  // $105.00

  EXPECT_TRUE(pq.pop());  // removes id=2 ($110.00)
  EXPECT_TRUE(pq.pop());  // removes id=3 ($105.00)
  EXPECT_TRUE(pq.pop());  // removes id=1 ($100.00)

  EXPECT_TRUE(pq.empty());
  EXPECT_FALSE(pq.pop());  // now empty
}

TEST(PriorityQueueAdapterTest, EmptyInitially) {
  PQ pq;
  EXPECT_TRUE(pq.empty());
  EXPECT_FALSE(pq.top().has_value());
}

TEST(PriorityQueueAdapterTest, PushAndTop) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));  // $100.00
  pq.push(flm::Order(2, 1050000, 10, flm::BUY_SIDE, "c2"));  // $105.00 — higher, goes to top

  ASSERT_TRUE(pq.top().has_value());
  // BuyOrderComparator puts highest price first; id=2 has the higher price
  EXPECT_EQ(pq.top()->id_ns, static_cast<uint64_t>(2) << 32);
}

TEST(PriorityQueueAdapterTest, PushInvalidOrderIgnored) {
  PQ pq;

  // volume=0 makes isValid() return false; push() rejects invalid orders
  pq.push(flm::Order(1, 1000000, 0, flm::BUY_SIDE, "c1"));
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapterTest, FindExistingAndMissing) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));

  // find() takes the raw int order_id, not id_ns
  auto found = pq.find(1);
  ASSERT_TRUE(found.has_value());
  EXPECT_EQ(found->id_ns, static_cast<uint64_t>(1) << 32);

  EXPECT_FALSE(pq.find(999).has_value());
}

TEST(PriorityQueueAdapterTest, PopRemovesTop) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));  // $100.00
  pq.push(flm::Order(2, 1050000, 10, flm::BUY_SIDE, "c2"));  // $105.00 — top

  ASSERT_EQ(pq.top()->id_ns, static_cast<uint64_t>(2) << 32);
  pq.pop();

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(pq.top()->id_ns, static_cast<uint64_t>(1) << 32);
}

TEST(PriorityQueueAdapterTest, PopOnEmptyDoesNothing) {
  PQ pq;

  EXPECT_NO_THROW(pq.pop());
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapterTest, RemoveExistingOrder) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));

  EXPECT_TRUE(pq.remove(1));
  EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueAdapterTest, RemoveNonExistingOrder) {
  PQ pq;

  EXPECT_FALSE(pq.remove(42));
}

TEST(PriorityQueueAdapterTest, RemoveUpdatesTopCorrectly) {
  PQ pq;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1"));  // $100.00
  pq.push(flm::Order(2, 1100000, 10, flm::BUY_SIDE, "c2"));  // $110.00 — top

  EXPECT_TRUE(pq.remove(2));

  ASSERT_TRUE(pq.top().has_value());
  EXPECT_EQ(pq.top()->id_ns, static_cast<uint64_t>(1) << 32);
}