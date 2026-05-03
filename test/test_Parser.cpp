#include <gtest/gtest.h>

#include <sstream>

#include "Parser.hpp"

namespace flm = fast_little_market;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// SOH delimiter as a string literal character.
#define SOH "\x01"

// Compute BodyLength (number of bytes from tag 9's SOH up to but not including
// the CheckSum field delimiter) for a given set of body fields.
static std::string bodyLen(std::initializer_list<std::string> bodyFields) {
  size_t n = 0;
  for (const auto& f : bodyFields) n += f.size() + 1;  // +1 for SOH
  return std::to_string(n);
}

// Build a minimal but structurally valid FIX 4.4 message.
//   header:  8=FIX.4.4 | 9=<len> | <body fields> | 10=000
// CheckSum value is intentionally left as "000" — the parser does not
// validate it, it just stops when it sees tag 10.
static std::string buildMsg(std::string_view msgType,
                            std::initializer_list<std::string> body) {
  const std::string bl =
      bodyLen({std::string("35=") + std::string(msgType),
               std::string("49=SENDER"), std::string("56=TARGET")});

  // Rebuild body length including all body fields properly.
  std::vector<std::string> allBody = {std::string("35=") + std::string(msgType),
                                      "49=SENDER", "56=TARGET"};
  for (const auto& f : body) allBody.push_back(f);

  size_t bodyBytes = 0;
  for (const auto& f : allBody) bodyBytes += f.size() + 1;

  std::string msg;
  msg += "8=FIX.4.4" SOH;
  msg += "9=" + std::to_string(bodyBytes) + SOH;
  for (const auto& f : allBody) msg += f + SOH;
  msg += "10=000" SOH;
  return msg;
}

static std::istringstream toStream(const std::string& s) {
  return std::istringstream(s);
}

// ---------------------------------------------------------------------------
// Basic parsing — header fields
// ---------------------------------------------------------------------------

TEST(ParserTest, ParsesBeginString) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto val = msg.getString(flm::fix::FieldTag::BeginString);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "FIX.4.4");
}

TEST(ParserTest, ParsesMsgType) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto val = msg.getString(flm::fix::FieldTag::MsgType);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "D");
}

TEST(ParserTest, ParsesSenderAndTargetCompID) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto sender = msg.getString(flm::fix::FieldTag::SenderCompID);
  auto target = msg.getString(flm::fix::FieldTag::TargetCompID);
  ASSERT_TRUE(sender.has_value());
  ASSERT_TRUE(target.has_value());
  EXPECT_EQ(*sender, "SENDER");
  EXPECT_EQ(*target, "TARGET");
}

TEST(ParserTest, ParsesBodyLength) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("0", {}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto val = msg.getInt(flm::fix::FieldTag::BodyLength);
  EXPECT_TRUE(val.has_value());
  EXPECT_GT(*val, 0);
}

// ---------------------------------------------------------------------------
// Body field parsing
// ---------------------------------------------------------------------------

TEST(ParserTest, ParsesIntField) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {"38=1000"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto val = msg.getInt(flm::fix::FieldTag::OrderQty);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 1000);
}

TEST(ParserTest, ParsesCharField) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {"54=1"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto val = msg.getChar(flm::fix::FieldTag::Side);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, '1');
}

TEST(ParserTest, ParsesStringField) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {"11=ORD-001"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto val = msg.getString(flm::fix::FieldTag::ClOrdID);
  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, "ORD-001");
}

TEST(ParserTest, ParsesMultipleBodyFields) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream =
      toStream(buildMsg("D", {"11=ORD-007", "38=500", "54=2", "55=AAPL"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  EXPECT_EQ(msg.getString(flm::fix::FieldTag::ClOrdID).value_or(""), "ORD-007");
  EXPECT_EQ(msg.getInt(flm::fix::FieldTag::OrderQty).value_or(0), 500);
  EXPECT_EQ(msg.getChar(flm::fix::FieldTag::Side).value_or('?'), '2');
  EXPECT_EQ(msg.getString(flm::fix::FieldTag::Symbol).value_or(""), "AAPL");
}

TEST(ParserTest, UnknownTagIsStoredAsInvalid) {
  // Tag 9999 does not exist in FieldTag — parser maps it to FieldTag::INVALID
  // and the field overwrites any prior INVALID entry without crashing.
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  auto stream = toStream(buildMsg("D", {"9999=X"}));
  // Should complete without throwing.
  EXPECT_NO_THROW(flm::fix::FixMessage::parse(stream, msg, defs));
}

// ---------------------------------------------------------------------------
// Reset between parses
// ---------------------------------------------------------------------------

TEST(ParserTest, SecondParseOverwritesFirstMessage) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;

  auto s1 = toStream(buildMsg("D", {"11=FIRST"}));
  flm::fix::FixMessage::parse(s1, msg, defs);
  EXPECT_EQ(msg.getString(flm::fix::FieldTag::ClOrdID).value_or(""), "FIRST");

  auto s2 = toStream(buildMsg("D", {"11=SECOND"}));
  flm::fix::FixMessage::parse(s2, msg, defs);
  EXPECT_EQ(msg.getString(flm::fix::FieldTag::ClOrdID).value_or(""), "SECOND");
}

TEST(ParserTest, FieldAbsentInSecondMessageIsNotVisible) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;

  auto s1 = toStream(buildMsg("D", {"11=ORD-1", "38=100"}));
  flm::fix::FixMessage::parse(s1, msg, defs);

  auto s2 = toStream(buildMsg("D", {"11=ORD-2"}));
  flm::fix::FixMessage::parse(s2, msg, defs);

  // OrderQty was in the first message but not the second.
  EXPECT_FALSE(msg.getInt(flm::fix::FieldTag::OrderQty).has_value());
}

// ---------------------------------------------------------------------------
// Truncated / empty stream
// ---------------------------------------------------------------------------

TEST(ParserTest, EmptyStreamDoesNotCrash) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  std::istringstream empty("");
  EXPECT_NO_THROW(flm::fix::FixMessage::parse(empty, msg, defs));
}

TEST(ParserTest, TruncatedStreamDoesNotCrash) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  // Fewer than the mandatory 37 bytes.
  std::istringstream stream("8=FIX.4.4" SOH "9=5" SOH);
  EXPECT_NO_THROW(flm::fix::FixMessage::parse(stream, msg, defs));
}

// ---------------------------------------------------------------------------
// Groups
// ---------------------------------------------------------------------------

TEST(ParserTest, SingleGroupIsParsed) {
  // NoLegs (555) = 1, followed by LegSymbol (600).
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  defs.add("s", {flm::fix::GroupDef{flm::fix::FieldTag::NoLegs,
                                    flm::fix::FieldTag::LegSymbol}});

  // MsgType "s" = NewOrderCross
  auto stream = toStream(buildMsg("s", {"555=1", "600=IBM"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto grp = msg.getGroup(flm::fix::FieldTag::NoLegs, 0);
  ASSERT_TRUE(grp.has_value());
  auto sym = grp->getString(flm::fix::FieldTag::LegSymbol);
  ASSERT_TRUE(sym.has_value());
  EXPECT_EQ(*sym, "IBM");
}

TEST(ParserTest, MultipleGroupsAreParsedInOrder) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  defs.add("s", {flm::fix::GroupDef{flm::fix::FieldTag::NoLegs,
                                    flm::fix::FieldTag::LegSymbol}});

  auto stream = toStream(buildMsg("s", {"555=2", "600=IBM", "600=MSFT"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto g0 = msg.getGroup(flm::fix::FieldTag::NoLegs, 0);
  auto g1 = msg.getGroup(flm::fix::FieldTag::NoLegs, 1);
  ASSERT_TRUE(g0.has_value());
  ASSERT_TRUE(g1.has_value());
  EXPECT_EQ(g0->getString(flm::fix::FieldTag::LegSymbol).value_or(""), "IBM");
  EXPECT_EQ(g1->getString(flm::fix::FieldTag::LegSymbol).value_or(""), "MSFT");
}

TEST(ParserTest, GroupCountMatchesNoLegsValue) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  defs.add("s", {flm::fix::GroupDef{flm::fix::FieldTag::NoLegs,
                                    flm::fix::FieldTag::LegSymbol}});

  auto stream = toStream(buildMsg("s", {"555=2", "600=A", "600=B"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto g0 = msg.getGroup(flm::fix::FieldTag::NoLegs, 0);
  auto g1 = msg.getGroup(flm::fix::FieldTag::NoLegs, 1);
  auto g2 = msg.getGroup(flm::fix::FieldTag::NoLegs, 2);

  EXPECT_TRUE(g0.has_value());
  EXPECT_TRUE(g1.has_value());
  EXPECT_FALSE(g2.has_value());
}

TEST(ParserTest, GroupOutOfRangeReturnsNullopt) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  defs.add("s", {flm::fix::GroupDef{flm::fix::FieldTag::NoLegs,
                                    flm::fix::FieldTag::LegSymbol}});

  auto stream = toStream(buildMsg("s", {"555=1", "600=X"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  EXPECT_FALSE(msg.getGroup(flm::fix::FieldTag::NoLegs, 1).has_value());
}

TEST(ParserTest, GroupFieldsAreIndependentAcrossSiblings) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  defs.add("s", {flm::fix::GroupDef{flm::fix::FieldTag::NoLegs,
                                    flm::fix::FieldTag::LegSymbol}});

  auto stream = toStream(buildMsg("s", {"555=2", "600=FIRST", "600=SECOND"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  auto g0 = msg.getGroup(flm::fix::FieldTag::NoLegs, 0);
  auto g1 = msg.getGroup(flm::fix::FieldTag::NoLegs, 1);
  ASSERT_TRUE(g0.has_value());
  ASSERT_TRUE(g1.has_value());
  // Each group has its own LegSymbol — values must not bleed across.
  EXPECT_NE(g0->getString(flm::fix::FieldTag::LegSymbol).value_or(""),
            g1->getString(flm::fix::FieldTag::LegSymbol).value_or(""));
}

TEST(ParserTest, NoGroupDefsResultsInNoGroupsParsed) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;  // no defs registered

  auto stream = toStream(buildMsg("s", {"555=1", "600=IBM"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  // Without a GroupDef, NoLegs is stored as a plain field, not a group.
  EXPECT_FALSE(msg.getGroup(flm::fix::FieldTag::NoLegs, 0).has_value());
  EXPECT_TRUE(msg.getInt(flm::fix::FieldTag::NoLegs).has_value());
}

TEST(ParserTest, GroupDefsForDifferentMsgTypeAreNotApplied) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  // Register the group def only for "V" (MarketDataRequest), not "D".
  defs.add("V", {flm::fix::GroupDef{flm::fix::FieldTag::NoLegs,
                                    flm::fix::FieldTag::LegSymbol}});

  auto stream = toStream(buildMsg("D", {"555=1", "600=IBM"}));
  flm::fix::FixMessage::parse(stream, msg, defs);

  EXPECT_FALSE(msg.getGroup(flm::fix::FieldTag::NoLegs, 0).has_value());
}