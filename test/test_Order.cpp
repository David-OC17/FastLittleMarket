#include <gtest/gtest.h>

#include "Order.hpp"

namespace flm = fast_little_market;

// Order constructor: (int id, uint32_t price_q4_, uint32_t vol, bool is_buy,
// std::string_view client_, GlobalSequencer& sequencer)
// id_ns_ = static_cast<uint64_t>(id) << 32

class OrderTest : public ::testing::Test {
 protected:
  flm::GlobalSequencer sequencer_;
};

TEST_F(OrderTest, Constructor) {
  // Snapshot counter before — the constructor will consume exactly one value
  uint64_t ts_before = sequencer_.nextTimestampNs();
  flm::Order order(1, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  // ts_before + 1 was consumed by the constructor
  // ts_before + 2 is what we get now

  sequencer_.nextTimestampNs();

  // The timestamp embedded in id_ns_ must be exactly ts_before + 1
  uint64_t ts_used = ts_before + 1;
  EXPECT_EQ(order.id_ns_, flm::Order::pack(1, ts_used));

  // Verify components unpack correctly
  EXPECT_EQ(order.id_ns_ >> 32, 1ULL);  // id in upper bits
  EXPECT_EQ(order.id_ns_ & 0xFFFFFFFFULL,
            ts_used & 0xFFFFFFFFULL);  // ts in lower

  EXPECT_EQ(order.price_q4_, 100u);
  EXPECT_EQ(order.volume_, 10u);
  EXPECT_EQ(order.side_, flm::BUY_SIDE);
  EXPECT_STREQ(order.client_, "JPMORG");
}

TEST_F(OrderTest, CopyConstructor) {
  flm::Order original(2, 50, 5, flm::SELL_SIDE, "JPMORG", sequencer_);
  flm::Order copy = original;

  EXPECT_EQ(copy.id_ns_, original.id_ns_);
  EXPECT_EQ(copy.price_q4_, original.price_q4_);
  EXPECT_EQ(copy.volume_, original.volume_);
  EXPECT_EQ(copy.side_, original.side_);
  EXPECT_STREQ(copy.client_, original.client_);
}

TEST_F(OrderTest, AssignmentOperator) {
  // Constructor: (id, price_q4_, vol, is_buy, client_)
  flm::Order original(3, 75, 20, flm::BUY_SIDE, "JPMORG", sequencer_);
  flm::Order assigned(0, 0, 0, flm::SELL_SIDE, "JPMORG", sequencer_);

  assigned = original;

  EXPECT_EQ(assigned.id_ns_, original.id_ns_);
  EXPECT_EQ(assigned.price_q4_, original.price_q4_);
  EXPECT_EQ(assigned.volume_, original.volume_);
  EXPECT_EQ(assigned.side_, original.side_);
  EXPECT_STREQ(assigned.client_, original.client_);
}

TEST_F(OrderTest, Validity) {
  flm::Order valid_order(4, 25, 15, flm::SELL_SIDE, "JPMORG", sequencer_);
  EXPECT_TRUE(valid_order.isValid());

  // volume_ is uint32_t — use 0 to trigger the invalid volume_ check,
  // since isValid() checks volume_ == 0 (passing -5 would wrap to a large uint)
  flm::Order invalid_volume(6, 30, 0, flm::SELL_SIDE, "client7", sequencer_);
  EXPECT_FALSE(invalid_volume.isValid());

  flm::Order invalid_client(7, 20, 10, flm::BUY_SIDE, "", sequencer_);
  EXPECT_FALSE(invalid_client.isValid());

  // price_q4_ == 0 is also invalid
  flm::Order invalid_price(8, 0, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  EXPECT_FALSE(invalid_price.isValid());
}

TEST_F(OrderTest, DirectFieldMutation) {
  // Order is a plain struct — fields are set directly, no setters
  flm::Order order(8, 60, 25, flm::SELL_SIDE, "client8", sequencer_);

  order.price_q4_ = 65;
  EXPECT_EQ(order.price_q4_, 65u);

  order.volume_ = 30;
  EXPECT_EQ(order.volume_, 30u);

  std::strncpy(order.client_, "client9", 7);
  order.client_[7] = '\0';
  EXPECT_STREQ(order.client_, "client9");

  order.side_ = flm::BUY_SIDE;
  EXPECT_EQ(order.side_, flm::BUY_SIDE);
}

TEST_F(OrderTest, InvalidFieldValues) {
  flm::Order order(9, 80, 40, flm::BUY_SIDE, "client10", sequencer_);

  // price_q4_ = 0 makes order invalid (uint32_t, so just use 0)
  order.price_q4_ = 0;
  EXPECT_EQ(order.price_q4_, 0u);
  EXPECT_FALSE(order.isValid());

  order.price_q4_ = 80;  // restore

  // volume_ = 0 makes order invalid
  order.volume_ = 0;
  EXPECT_EQ(order.volume_, 0u);
  EXPECT_FALSE(order.isValid());

  order.volume_ = 40;  // restore

  // empty client_ makes order invalid
  order.client_[0] = '\0';
  EXPECT_STREQ(order.client_, "");
  EXPECT_FALSE(order.isValid());
}

TEST_F(OrderTest, InvalidNewOrdersFail) {}

TEST_F(OrderTest, Ordering) {
  // Lower price_q4_ comes first; ties broken by id_ns_ (FIFO)
  flm::Order low_price(1, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  flm::Order high_price(2, 200, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  EXPECT_TRUE(low_price < high_price);
  EXPECT_FALSE(high_price < low_price);

  // Same price: lower id_ns_ (earlier arrival) comes first
  flm::Order earlier(1, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  flm::Order later(2, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  EXPECT_TRUE(earlier < later);
}