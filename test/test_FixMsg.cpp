#include <gtest/gtest.h>

#include <bitset>

#include "MsgType.hpp"

namespace flm = fast_little_market;

struct Case {
  flm::fix::MsgType t_;
  char c_;
  std::string_view name_;
};

// NOTE: Manually verify the CASES. Keep updated to used fix standard.
constexpr Case CASES[] = {
    {flm::fix::MsgType::Heartbeat, '0', "Heartbeat"},
    {flm::fix::MsgType::NewOrderSingle, 'D', "NewOrderSingle"},
    {flm::fix::MsgType::QuoteCancel, 'Z', "QuoteCancel"},
    // TODO: add rest of the message types
};

TEST(FixMsgTest, MsgTypeCharMapping) {
  for (auto& c : CASES) {
    EXPECT_EQ(static_cast<char>(c.t_), c.c_);
  }
}

TEST(FixMsgTest, MsgTypeToString) {
  for (auto& c : CASES) {
    EXPECT_EQ(flm::fix::mtos(c.t_), c.name_);
  }
}

TEST(FixMsgTest, StringToMsgType) {
  for (auto& c : CASES) {
    EXPECT_EQ(flm::fix::stom(c.name_), c.t_);
  }
}