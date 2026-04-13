#include <gtest/gtest.h>

#include "Order.hpp"

namespace flm = FastLittleMarket;

// Order constructor: (int id, uint32_t price_q4, uint32_t vol, bool is_buy,
// std::string_view client)
// id_ns = static_cast<uint64_t>(id) << 32

TEST(OrderTest, Constructor) {
  flm::Order order(1, 100, 10, flm::BUY_SIDE, "JPMORG");

  EXPECT_EQ(order.id_ns, static_cast<uint64_t>(1) << 32);
  EXPECT_EQ(order.price_q4, 100u);
  EXPECT_EQ(order.volume, 10u);
  EXPECT_EQ(order.side, flm::BUY_SIDE);
  EXPECT_STREQ(order.client, "JPMORG");
}

TEST(OrderTest, CopyConstructor) {
  flm::Order original(2, 50, 5, flm::SELL_SIDE, "JPMORG");
  flm::Order copy = original;

  EXPECT_EQ(copy.id_ns, original.id_ns);
  EXPECT_EQ(copy.price_q4, original.price_q4);
  EXPECT_EQ(copy.volume, original.volume);
  EXPECT_EQ(copy.side, original.side);
  EXPECT_STREQ(copy.client, original.client);
}

TEST(OrderTest, AssignmentOperator) {
  // Constructor: (id, price_q4, vol, is_buy, client)
  flm::Order original(3, 75, 20, flm::BUY_SIDE, "JPMORG");
  flm::Order assigned(0, 0, 0, flm::SELL_SIDE, "JPMORG");

  assigned = original;

  EXPECT_EQ(assigned.id_ns, original.id_ns);
  EXPECT_EQ(assigned.price_q4, original.price_q4);
  EXPECT_EQ(assigned.volume, original.volume);
  EXPECT_EQ(assigned.side, original.side);
  EXPECT_STREQ(assigned.client, original.client);
}

TEST(OrderTest, Validity) {
  flm::Order valid_order(4, 25, 15, flm::SELL_SIDE, "JPMORG");
  EXPECT_TRUE(valid_order.isValid());

  // volume is uint32_t — use 0 to trigger the invalid volume check,
  // since isValid() checks volume == 0 (passing -5 would wrap to a large uint)
  flm::Order invalid_volume(6, 30, 0, flm::SELL_SIDE, "client7");
  EXPECT_FALSE(invalid_volume.isValid());

  flm::Order invalid_client(7, 20, 10, flm::BUY_SIDE, "");
  EXPECT_FALSE(invalid_client.isValid());

  // price_q4 == 0 is also invalid
  flm::Order invalid_price(8, 0, 10, flm::BUY_SIDE, "JPMORG");
  EXPECT_FALSE(invalid_price.isValid());
}

TEST(OrderTest, DirectFieldMutation) {
  // Order is a plain struct — fields are set directly, no setters
  flm::Order order(8, 60, 25, flm::SELL_SIDE, "client8");

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

TEST(OrderTest, InvalidFieldValues) {
  // Fields can be set to invalid values — isValid() catches them at check time
  flm::Order order(9, 80, 40, flm::BUY_SIDE, "client10");

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

TEST(OrderTest, Ordering) {
  // Lower price_q4 comes first; ties broken by id_ns (FIFO)
  flm::Order low_price(1, 100, 10, flm::BUY_SIDE, "JPMORG");
  flm::Order high_price(2, 200, 10, flm::BUY_SIDE, "JPMORG");
  EXPECT_TRUE(low_price < high_price);
  EXPECT_FALSE(high_price < low_price);

  // Same price: lower id_ns (earlier arrival) comes first
  flm::Order earlier(1, 100, 10, flm::BUY_SIDE, "JPMORG");
  flm::Order later(2, 100, 10, flm::BUY_SIDE, "JPMORG");
  EXPECT_TRUE(earlier < later);
}

TEST(OrderTest, Equality) {
  flm::Order a(5, 100, 10, flm::BUY_SIDE, "JPMORG");
  flm::Order b(5, 200, 99, flm::SELL_SIDE,
               "OTHER");  // same id, different fields
  flm::Order c(6, 100, 10, flm::BUY_SIDE, "JPMORG");  // different id

  // Equality is based solely on id_ns
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
}