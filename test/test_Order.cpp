#include <gtest/gtest.h>

#include "Order.hpp"

namespace flm = FastLittleMarket;

TEST(OrderTest, Constructor) {
  flm::Order order(1, flm::OrderSide::Buy, 100.5, 10, "client1");

  EXPECT_EQ(order.getId(), 1);
  EXPECT_DOUBLE_EQ(order.getPrice(), 100.5);
  EXPECT_EQ(order.getVolume(), 10);
  EXPECT_EQ(order.getSide(), flm::OrderSide::Buy);
  EXPECT_EQ(order.getClient(), "client1");
}

TEST(OrderTest, CopyConstructor) {
  flm::Order original(2, flm::OrderSide::Sell, 50.0, 5, "client2");
  flm::Order copy = original;

  EXPECT_EQ(copy.getId(), original.getId());
  EXPECT_DOUBLE_EQ(copy.getPrice(), original.getPrice());
  EXPECT_EQ(copy.getVolume(), original.getVolume());
  EXPECT_EQ(copy.getSide(), original.getSide());
  EXPECT_EQ(copy.getClient(), original.getClient());
}

TEST(OrderTest, AssignmentOperator) {
  flm::Order original(3, flm::OrderSide::Buy, 75.0, 20, "client3");
  flm::Order assigned(0, flm::OrderSide::Sell, 0.0, 0, "client4");

  assigned = original;

  EXPECT_EQ(assigned.getId(), original.getId());
  EXPECT_DOUBLE_EQ(assigned.getPrice(), original.getPrice());
  EXPECT_EQ(assigned.getVolume(), original.getVolume());
  EXPECT_EQ(assigned.getSide(), original.getSide());
  EXPECT_EQ(assigned.getClient(), original.getClient());
}

TEST(OrderTest, Validity) {
  flm::Order valid_order(4, flm::OrderSide::Sell, 25.0, 15, "client5");
  EXPECT_TRUE(valid_order.isValid());

  flm::Order invalid_volume(6, flm::OrderSide::Sell, 30.0, -5, "client7");
  EXPECT_FALSE(invalid_volume.isValid());

  flm::Order invalid_client(7, flm::OrderSide::Buy, 20.0, 10, "");
  EXPECT_FALSE(invalid_client.isValid());
}

TEST(OrderTest, Setters) {
  flm::Order order(8, flm::OrderSide::Sell, 60.0, 25, "client8");

  order.setPrice(65.0);
  EXPECT_DOUBLE_EQ(order.getPrice(), 65.0);

  order.setVolume(30);
  EXPECT_EQ(order.getVolume(), 30);

  order.setClient("client9");
  EXPECT_EQ(order.getClient(), "client9");

  order.setSide(flm::OrderSide::Buy);
  EXPECT_EQ(order.getSide(), flm::OrderSide::Buy);
}

TEST(OrderTest, InvalidSetters) {
  flm::Order order(9, flm::OrderSide::Buy, 80.0, 40, "client10");

  order.setPrice(-5.0);
  EXPECT_DOUBLE_EQ(order.getPrice(), -5.0);  // Still set, but invalid

  order.setVolume(-10);
  EXPECT_EQ(order.getVolume(), -10);  // Still set, but invalid

  order.setClient("");
  EXPECT_EQ(order.getClient(), "");  // Still set, but invalid
}

TEST(OrderTest, Timestamp) {
  flm::Order order(10, flm::OrderSide::Sell, 55.0, 10, "client11");
  auto timestamp = order.getTimestamp();

  // Check that the timestamp is reasonably close to now
  auto now = std::chrono::system_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);
  EXPECT_LE(duration.count(),
            5);  // Timestamp should be within the last 5 seconds
}