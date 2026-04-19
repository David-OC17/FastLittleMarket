#include <gtest/gtest.h>

#include <bitset>

#include "FixMsg.hpp"

namespace flm = fast_little_market;

struct Case {
  flm::fix::MsgType t;
  char c;
  std::string_view name;
};

// NOTE: Manually verify the cases. Keep updated to used fix standard.
constexpr Case cases[] = {
    {flm::fix::MsgType::Heartbeat, '0', "Heartbeat"},
    {flm::fix::MsgType::NewOrderSingle, 'D', "NewOrderSingle"},
    {flm::fix::MsgType::QuoteCancel, 'Z', "QuoteCancel"},
    // TODO: add rest of the message types
};

TEST(FixMsgTest, MsgTypeCharMapping) {
  for (auto& c : cases) {
    EXPECT_EQ(static_cast<char>(c.t), c.c);
  }
}

TEST(FixMsgTest, MsgTypeToString) {
  for (auto& c : cases) {
    EXPECT_EQ(flm::fix::mtos(c.t), c.name);
  }
}

TEST(FixMsgTest, StringToMsgType) {
  for (auto& c : cases) {
    EXPECT_EQ(flm::fix::stom(c.name), c.t);
  }
}