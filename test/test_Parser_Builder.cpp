#include <gtest/gtest.h>

#include <regex>
#include <sstream>

#include "Builder.hpp"
#include "FieldTag.hpp"
#include "Parser.hpp"

namespace flm = fast_little_market;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

const std::string sample_new_order_single =
    "8=FIX.5.0^A9=146^A35=D^A34=4^A49=ABC_DEFG01^A52=20090323-15:40:29^A56="
    "CCG^A115=XYZ^A11=NF 0542/03232009^A54=1^A38=100^A55=CVS^A40=1^A59=0^A47="
    "A^A60=20090323-15:40:29^A21=1^A207=N^A10=195^A";
const std::string sample_cancel_replace =
    "8=FIX.5.0^A9=197^A35=G^A34=118^A49=ABC_DEFG01^A52=20090325-15:14:47^A56="
    "CCG^A115=XYZ^A11=NF 0574/03252009^A37=NF 0573/03252009^A41=NF "
    "0573/03252009^A54=1^A38=2000^A55=CVS^A40=2^A44=25.47^A59=0^A47=A^A60="
    "20090325-15:14:47^A21=1^A207=N^A10=185^A";
const std::string sample_logon =
    "8=FIX.4.4^A9=75^A35=A^A34=1092^A49=TESTBUY1^A52=20180920-18:24:59.643^"
    "A56=TESTSELL1^A98=0^A108=60^A10=178^A";
const std::string sample_logout =
    "8=FIX.4.4^A9=63^A35=5^A34=1091^A49=TESTBUY1^A52=20180920-18:24:58.675^"
    "A56=TESTSELL1^A10=138^A";
const std::string sample_limit_order =
    "8=FIX.5.0^A9=152^A35=D^A34=7332^A49=TEST1^A52=20160208-14:41:33.643^A56="
    "DWFIX01^A1=DPQP000013^A11=982A298766020822123456797^A21=1^A38=170^A40=1^"
    "A44=0.01^A54=1^A55=AMZN^A60=20160208-14:41:33.643^A10=243^A";

const std::regex decode_reg("\\^A");
const std::regex encode_reg("\\\x01");

inline std::string decodeFix(const std::string& sample) {
  return std::regex_replace(sample, decode_reg, "\x01");
}
inline std::string encodeFix(const std::string_view& str) {
  return std::regex_replace(std::string(str), encode_reg, "^A");
}

static flm::fix::GroupDefs noGroupDefs() { return {}; }

// ---------------------------------------------------------------------------
// Parser — basic field extraction
// ---------------------------------------------------------------------------

TEST(RoundTripTest, ParserExtractsBeginString) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_new_order_single), msg,
                              noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::BeginString);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "FIX.5.0");
}

TEST(RoundTripTest, ParserExtractsMsgType) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_new_order_single), msg,
                              noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::MsgType);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "D");
}

TEST(RoundTripTest, ParserExtractsClOrdID) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_new_order_single), msg,
                              noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::ClOrdID);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "NF 0542/03232009");
}

TEST(RoundTripTest, ParserExtractsIntField) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_new_order_single), msg,
                              noGroupDefs());
  auto val = msg.getInt(flm::fix::FieldTag::OrderQty);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 100);
}

TEST(RoundTripTest, ParserExtractsSymbol) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_new_order_single), msg,
                              noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::Symbol);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "CVS");
}

TEST(RoundTripTest, ParserReturnsNulloptForMissingField) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_logon), msg, noGroupDefs());
  EXPECT_FALSE(msg.getString(flm::fix::FieldTag::ClOrdID).has_value());
}

// ---------------------------------------------------------------------------
// Parser — different message types
// ---------------------------------------------------------------------------

TEST(RoundTripTest, LogonMsgTypeIsA) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_logon), msg, noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::MsgType);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "A");
}

TEST(RoundTripTest, LogoutMsgTypeIs5) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_logout), msg, noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::MsgType);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "5");
}

TEST(RoundTripTest, CancelReplaceExtractsQty) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_cancel_replace), msg,
                              noGroupDefs());
  auto val = msg.getInt(flm::fix::FieldTag::OrderQty);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 2000);
}

TEST(RoundTripTest, LimitOrderExtractsLongClOrdID) {
  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_limit_order), msg,
                              noGroupDefs());
  auto val = msg.getString(flm::fix::FieldTag::ClOrdID);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "982A298766020822123456797");
}

// ---------------------------------------------------------------------------
// Builder — field output
// ---------------------------------------------------------------------------

TEST(RoundTripTest, BuilderProducesCorrectStringField) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, std::string_view("NF 0542/03232009"));
  auto msg = b.build();
  EXPECT_NE(msg.find("NF 0542/03232009"), std::string_view::npos);
}

TEST(RoundTripTest, BuilderProducesCorrectIntField) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(100));
  auto msg = b.build();
  EXPECT_NE(msg.find("100"), std::string_view::npos);
}

TEST(RoundTripTest, BuilderFieldsAreSOHDelimited) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, std::string_view("ABC"));
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(100));
  auto msg = b.build();
  auto encoded = encodeFix(msg);
  EXPECT_NE(encoded.find("^A"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Parser → builder → flm::fix::FixMessage::parser round trip
// ---------------------------------------------------------------------------

TEST(RoundTripTest, ParseBuildParsePreservesClOrdID) {
  flm::fix::GroupDefs defs = noGroupDefs();

  flm::fix::FixMessage msg1;
  flm::fix::FixMessage::parse(decodeFix(sample_new_order_single), msg1, defs);
  auto clord_id = msg1.getString(flm::fix::FieldTag::ClOrdID);
  ASSERT_TRUE(clord_id.has_value());

  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, *clord_id);
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(100));
  const std::string built{b.build()};

  flm::fix::FixMessage msg2;
  flm::fix::FixMessage::parse(built, msg2, defs);
  auto clord_id2 = msg2.getString(flm::fix::FieldTag::ClOrdID);
  ASSERT_TRUE(clord_id2.has_value());
  EXPECT_EQ(*clord_id, *clord_id2);
}

TEST(RoundTripTest, ParseBuildParsePreservesOrderQty) {
  flm::fix::GroupDefs defs = noGroupDefs();

  flm::fix::FixMessage msg1;
  flm::fix::FixMessage::parse(decodeFix(sample_limit_order), msg1, defs);
  auto qty = msg1.getInt(flm::fix::FieldTag::OrderQty);
  ASSERT_TRUE(qty.has_value());
  EXPECT_EQ(*qty, 170);

  flm::fix::FixMessage msg2;
  flm::fix::FixMessage::parse(decodeFix(sample_limit_order), msg2, defs);
  auto qty2 = msg2.getInt(flm::fix::FieldTag::OrderQty);
  ASSERT_TRUE(qty2.has_value());
  EXPECT_EQ(*qty, *qty2);
}

TEST(RoundTripTest, ParseBuildParsePreservesSymbol) {
  flm::fix::GroupDefs defs = noGroupDefs();

  flm::fix::FixMessage msg1;
  flm::fix::FixMessage::parse(decodeFix(sample_limit_order), msg1, defs);
  auto sym = msg1.getString(flm::fix::FieldTag::Symbol);
  ASSERT_TRUE(sym.has_value());

  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::Symbol, *sym);
  const std::string built{b.build()};

  flm::fix::FixMessage msg2;
  flm::fix::FixMessage::parse(built, msg2, defs);
  auto sym2 = msg2.getString(flm::fix::FieldTag::Symbol);
  ASSERT_TRUE(sym2.has_value());
  EXPECT_EQ(*sym, *sym2);
}

TEST(RoundTripTest, ParseBuildParsePreservesMultipleFields) {
  flm::fix::GroupDefs defs = noGroupDefs();

  flm::fix::FixMessage msg1;
  flm::fix::FixMessage::parse(decodeFix(sample_cancel_replace), msg1, defs);
  auto clord_id = msg1.getString(flm::fix::FieldTag::ClOrdID);
  auto qty = msg1.getInt(flm::fix::FieldTag::OrderQty);
  auto symbol = msg1.getString(flm::fix::FieldTag::Symbol);
  ASSERT_TRUE(clord_id.has_value());
  ASSERT_TRUE(qty.has_value());
  ASSERT_TRUE(symbol.has_value());

  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, *clord_id);
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(*qty));
  b.addField(flm::fix::FieldTag::Symbol, *symbol);
  const std::string built{b.build()};

  flm::fix::FixMessage msg2;
  flm::fix::FixMessage::parse(built, msg2, defs);
  EXPECT_EQ(msg2.getString(flm::fix::FieldTag::ClOrdID).value_or(""),
            *clord_id);
  EXPECT_EQ(msg2.getInt(flm::fix::FieldTag::OrderQty).value_or(0), *qty);
  EXPECT_EQ(msg2.getString(flm::fix::FieldTag::Symbol).value_or(""), *symbol);
}

// ---------------------------------------------------------------------------
// String → flm::fix::FixMessage::parser → builder → string
// ---------------------------------------------------------------------------

TEST(RoundTripTest, LogonFieldsPreservedAfterRebuild) {
  flm::fix::GroupDefs defs = noGroupDefs();

  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_logon), msg, defs);
  auto sender = msg.getString(flm::fix::FieldTag::SenderCompID);
  auto target = msg.getString(flm::fix::FieldTag::TargetCompID);
  auto msg_type = msg.getString(flm::fix::FieldTag::MsgType);
  ASSERT_TRUE(sender.has_value());
  ASSERT_TRUE(target.has_value());
  ASSERT_TRUE(msg_type.has_value());

  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::MsgType, *msg_type);
  b.addField(flm::fix::FieldTag::SenderCompID, *sender);
  b.addField(flm::fix::FieldTag::TargetCompID, *target);
  const auto built = encodeFix(b.build());

  EXPECT_NE(built.find("TESTBUY1"), std::string::npos);
  EXPECT_NE(built.find("TESTSELL1"), std::string::npos);
  EXPECT_NE(built.find("A"), std::string::npos);
}

TEST(RoundTripTest, LimitOrderCriticalFieldsSurviveRebuild) {
  flm::fix::GroupDefs defs = noGroupDefs();

  flm::fix::FixMessage msg;
  flm::fix::FixMessage::parse(decodeFix(sample_limit_order), msg, defs);
  auto clord_id = msg.getString(flm::fix::FieldTag::ClOrdID);
  auto qty = msg.getInt(flm::fix::FieldTag::OrderQty);
  auto symbol = msg.getString(flm::fix::FieldTag::Symbol);
  auto side = msg.getChar(flm::fix::FieldTag::Side);
  ASSERT_TRUE(clord_id.has_value());
  ASSERT_TRUE(qty.has_value());
  ASSERT_TRUE(symbol.has_value());
  ASSERT_TRUE(side.has_value());

  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, *clord_id);
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(*qty));
  b.addField(flm::fix::FieldTag::Symbol, *symbol);
  b.addField(flm::fix::FieldTag::Side, *side);
  const auto built = encodeFix(b.build());

  EXPECT_NE(built.find("982A298766020822123456797"), std::string::npos);
  EXPECT_NE(built.find("170"), std::string::npos);
  EXPECT_NE(built.find("AMZN"), std::string::npos);
}