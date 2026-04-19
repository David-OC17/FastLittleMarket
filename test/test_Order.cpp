#include <gtest/gtest.h>

#include "Order.hpp"

namespace flm = fast_little_market;

// Order constructor: (int id, uint32_t price_q4, uint32_t vol, bool is_buy,
// std::string_view client, GlobalSequencer& sequencer)
// id_ns = static_cast<uint64_t>(id) << 32

class OrderTest : public ::testing::Test {
 protected:
  flm::GlobalSequencer sequencer_;
};

TEST_F(OrderTest, Constructor) {
  // Snapshot counter before — the constructor will consume exactly one value
  uint64_t ts_before = sequencer_.next_timestamp_ns();
  flm::Order order(1, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  // ts_before + 1 was consumed by the constructor
  // ts_before + 2 is what we get now

  sequencer_.next_timestamp_ns();

  // The timestamp embedded in id_ns must be exactly ts_before + 1
  uint64_t ts_used = ts_before + 1;
  EXPECT_EQ(order.id_ns, flm::Order::pack(1, ts_used));

  // Verify components unpack correctly
  EXPECT_EQ(order.id_ns >> 32, 1ULL);  // id in upper bits
  EXPECT_EQ(order.id_ns & 0xFFFFFFFFULL,
            ts_used & 0xFFFFFFFFULL);  // ts in lower

  EXPECT_EQ(order.price_q4, 100u);
  EXPECT_EQ(order.volume, 10u);
  EXPECT_EQ(order.side, flm::BUY_SIDE);
  EXPECT_STREQ(order.client, "JPMORG");
}

TEST_F(OrderTest, CopyConstructor) {
  flm::Order original(2, 50, 5, flm::SELL_SIDE, "JPMORG", sequencer_);
  flm::Order copy = original;

  EXPECT_EQ(copy.id_ns, original.id_ns);
  EXPECT_EQ(copy.price_q4, original.price_q4);
  EXPECT_EQ(copy.volume, original.volume);
  EXPECT_EQ(copy.side, original.side);
  EXPECT_STREQ(copy.client, original.client);
}

TEST_F(OrderTest, AssignmentOperator) {
  // Constructor: (id, price_q4, vol, is_buy, client)
  flm::Order original(3, 75, 20, flm::BUY_SIDE, "JPMORG", sequencer_);
  flm::Order assigned(0, 0, 0, flm::SELL_SIDE, "JPMORG", sequencer_);

  assigned = original;

  EXPECT_EQ(assigned.id_ns, original.id_ns);
  EXPECT_EQ(assigned.price_q4, original.price_q4);
  EXPECT_EQ(assigned.volume, original.volume);
  EXPECT_EQ(assigned.side, original.side);
  EXPECT_STREQ(assigned.client, original.client);
}

TEST_F(OrderTest, Validity) {
  flm::Order valid_order(4, 25, 15, flm::SELL_SIDE, "JPMORG", sequencer_);
  EXPECT_TRUE(valid_order.isValid());

  // volume is uint32_t — use 0 to trigger the invalid volume check,
  // since isValid() checks volume == 0 (passing -5 would wrap to a large uint)
  flm::Order invalid_volume(6, 30, 0, flm::SELL_SIDE, "client7", sequencer_);
  EXPECT_FALSE(invalid_volume.isValid());

  flm::Order invalid_client(7, 20, 10, flm::BUY_SIDE, "", sequencer_);
  EXPECT_FALSE(invalid_client.isValid());

  // price_q4 == 0 is also invalid
  flm::Order invalid_price(8, 0, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  EXPECT_FALSE(invalid_price.isValid());
}

TEST_F(OrderTest, DirectFieldMutation) {
  // Order is a plain struct — fields are set directly, no setters
  flm::Order order(8, 60, 25, flm::SELL_SIDE, "client8", sequencer_);

  order.price_q4 = 65;
  EXPECT_EQ(order.price_q4, 65u);

  order.volume = 30;
  EXPECT_EQ(order.volume, 30u);

  std::strncpy(order.client, "client9", 7);
  order.client[7] = '\0';
  EXPECT_STREQ(order.client, "client9");

  order.side = flm::BUY_SIDE;
  EXPECT_EQ(order.side, flm::BUY_SIDE);
}

TEST_F(OrderTest, InvalidFieldValues) {
  flm::Order order(9, 80, 40, flm::BUY_SIDE, "client10", sequencer_);

  // price_q4 = 0 makes order invalid (uint32_t, so just use 0)
  order.price_q4 = 0;
  EXPECT_EQ(order.price_q4, 0u);
  EXPECT_FALSE(order.isValid());

  order.price_q4 = 80;  // restore

  // volume = 0 makes order invalid
  order.volume = 0;
  EXPECT_EQ(order.volume, 0u);
  EXPECT_FALSE(order.isValid());

  order.volume = 40;  // restore

  // empty client makes order invalid
  order.client[0] = '\0';
  EXPECT_STREQ(order.client, "");
  EXPECT_FALSE(order.isValid());
}

TEST_F(OrderTest, InvalidNewOrdersFail) {

}

TEST_F(OrderTest, Ordering) {
  // Lower price_q4 comes first; ties broken by id_ns (FIFO)
  flm::Order low_price(1, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  flm::Order high_price(2, 200, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  EXPECT_TRUE(low_price < high_price);
  EXPECT_FALSE(high_price < low_price);

  // Same price: lower id_ns (earlier arrival) comes first
  flm::Order earlier(1, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  flm::Order later(2, 100, 10, flm::BUY_SIDE, "JPMORG", sequencer_);
  EXPECT_TRUE(earlier < later);
}