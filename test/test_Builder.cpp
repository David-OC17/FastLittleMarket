#include <gtest/gtest.h>

#include "Builder.hpp"
#include "FieldTag.hpp"

namespace flm = fast_little_market;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static std::string_view build(flm::fix::Builder& b) { return b.build(); }

static bool contains(std::string_view msg, std::string_view field) {
  return msg.find(field) != std::string_view::npos;
}

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

TEST(BuilderTest, DefaultConstructorProducesEmptyBuild) {
  flm::fix::Builder b;
  auto msg = build(b);
  // only checksum written with no fields
  EXPECT_FALSE(msg.empty());
}

TEST(BuilderTest, CustomMaxSizeIsRespected) {
  flm::fix::Builder b(256);
  EXPECT_NO_THROW(b.addField(flm::fix::FieldTag::ClOrdID, "ABC"));
}

TEST(BuilderTest, CopyConstructorIsDeleted) {
  EXPECT_FALSE(std::is_copy_constructible_v<flm::fix::Builder>);
}

TEST(BuilderTest, CopyAssignmentIsDeleted) {
  EXPECT_FALSE(std::is_copy_assignable_v<flm::fix::Builder>);
}

// ---------------------------------------------------------------------------
// addField — string_view
// ---------------------------------------------------------------------------

TEST(BuilderTest, AddFieldStringWritesTagEqualsValueSOH) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "ORDER1");
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "11=ORDER1\x01"));
}

TEST(BuilderTest, AddFieldStringMultipleFields) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "ABC")
      .addField(flm::fix::FieldTag::OrderQty, "100");
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "11=ABC\x01"));
  EXPECT_TRUE(contains(msg, "38=100\x01"));
}

TEST(BuilderTest, AddFieldStringEmptyValue) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "");
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "11=\x01"));
}

// ---------------------------------------------------------------------------
// addField — int64_t
// ---------------------------------------------------------------------------

TEST(BuilderTest, AddFieldIntPositive) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(42));
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "38=42\x01"));
}

TEST(BuilderTest, AddFieldIntNegative) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(-5));
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "38=-5\x01"));
}

TEST(BuilderTest, AddFieldIntZero) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(0));
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "38=0\x01"));
}

TEST(BuilderTest, AddFieldIntLargeValue) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::OrderQty, int64_t(9999999999LL));
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "38=9999999999\x01"));
}

// ---------------------------------------------------------------------------
// addField — char
// ---------------------------------------------------------------------------

TEST(BuilderTest, AddFieldCharWritesSingleChar) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::OrdType, '2');
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "40=2\x01"));
}

// ---------------------------------------------------------------------------
// checksum
// ---------------------------------------------------------------------------

TEST(BuilderTest, BuildAlwaysEndsWithChecksumField) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "X");
  auto msg = build(b);
  EXPECT_TRUE(contains(
      msg, std::string(flm::fix::ftof(flm::fix::FieldTag::CheckSum)) + "="));
}

TEST(BuilderTest, ChecksumIsCorrect) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "A");
  auto msg = build(b);

  std::string csTag =
      std::string(flm::fix::ftof(flm::fix::FieldTag::CheckSum)) + "=";
  auto csPos = msg.rfind(csTag);
  ASSERT_NE(csPos, std::string_view::npos);

  // compute expected sum over everything before the checksum field
  unsigned int sum = 0;
  for (size_t i = 0; i < csPos; i++) sum += static_cast<unsigned char>(msg[i]);
  sum %= 256;

  // extract actual checksum value from message (read until end or \x01)
  auto valueStart = csPos + csTag.size();
  auto valueEnd = msg.find('\x01', valueStart);
  if (valueEnd == std::string_view::npos) valueEnd = msg.size();
  int actual =
      std::stoi(std::string(msg.substr(valueStart, valueEnd - valueStart)));

  EXPECT_EQ(actual, (int)sum);
}

// ---------------------------------------------------------------------------
// reset
// ---------------------------------------------------------------------------

TEST(BuilderTest, ResetClearsFields) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "ORDER1");
  b.reset();
  auto msg = build(b);
  EXPECT_FALSE(contains(msg, "11=ORDER1"));
}

TEST(BuilderTest, ResetAllowsRebuild) {
  flm::fix::Builder b;
  b.addField(flm::fix::FieldTag::ClOrdID, "FIRST");
  b.reset();
  b.addField(flm::fix::FieldTag::ClOrdID, "SECOND");
  auto msg = build(b);
  EXPECT_TRUE(contains(msg, "11=SECOND\x01"));
}

// ---------------------------------------------------------------------------
// capacity
// ---------------------------------------------------------------------------

TEST(BuilderTest, ExceedingCapacityThrows) {
  flm::fix::Builder b(32);  // very small
  EXPECT_THROW(
      {
        for (int i = 0; i < 100; i++)
          b.addField(flm::fix::FieldTag::ClOrdID, "LONGVALUE");
      },
      std::length_error);
}

// ---------------------------------------------------------------------------
// append
// ---------------------------------------------------------------------------

TEST(BuilderTest, AppendCopiesFieldsFromSource) {
  flm::fix::Builder dst;
  flm::fix::Builder src;
  src.addField(flm::fix::FieldTag::ClOrdID, "X");
  dst.append(src);
  auto msg = build(dst);
  EXPECT_TRUE(contains(msg, "11=X\x01"));
}

TEST(BuilderTest, AppendResetsSrc) {
  flm::fix::Builder dst;
  flm::fix::Builder src;
  src.addField(flm::fix::FieldTag::ClOrdID, "X");
  dst.append(src);
  auto srcMsg = build(src);
  EXPECT_FALSE(contains(srcMsg, "11=X"));
}

// ---------------------------------------------------------------------------
// addTime
// ---------------------------------------------------------------------------

TEST(BuilderTest, AddTimeWritesExpectedFormat) {
  flm::fix::Builder b;
  timeval tv{};
  tv.tv_sec = 1700000000;  // a known UTC timestamp
  tv.tv_usec = 123000;
  b.addField(flm::fix::FieldTag::ClOrdID, "X");  // just to have a field
  b.addTime(flm::fix::FieldTag::TransactTime, tv);
  auto msg = build(b);
  // Format: YYYYMMDD-HH:MM:SS.mmm
  auto pos = msg.find("60=");
  ASSERT_NE(pos, std::string_view::npos);
  auto end = msg.find(SOH, pos + 3);
  auto timeStr = msg.substr(pos + 3, end - pos - 3);
  EXPECT_EQ(timeStr.size(), 21u);
  EXPECT_EQ(timeStr[8], '-');
  EXPECT_EQ(timeStr[11], ':');
  EXPECT_EQ(timeStr[14], ':');
  EXPECT_EQ(timeStr[17], '.');
}