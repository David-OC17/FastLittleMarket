#include <gtest/gtest.h>

#include "FieldMap.hpp"

namespace flm = fast_little_market;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static constexpr size_t DEFAULT_BUF = 4096;

static flm::fix::Field makeField(flm::fix::FieldTag tag, size_t offset,
                                 size_t length) {
  return flm::fix::Field(tag, offset, length);
}

// ---------------------------------------------------------------------------
// FieldMap::set / get
// ---------------------------------------------------------------------------

TEST(FieldMapTest, GetReturnsNullptrWhenEmpty) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "";
  flm::fix::FieldMap fm(buf, msg);

  EXPECT_EQ(fm.get(flm::fix::FieldTag::ClOrdID), nullptr);
}

TEST(FieldMapTest, SetAndGetRoundTrip) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "hello";
  flm::fix::FieldMap fm(buf, msg);

  flm::fix::Field f = makeField(flm::fix::FieldTag::ClOrdID, 0, 5);
  fm.set(f);

  auto result = fm.get(flm::fix::FieldTag::ClOrdID);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->tag_, flm::fix::FieldTag::ClOrdID);
  EXPECT_EQ(result->offset_, 0u);
  EXPECT_EQ(result->length_, 5u);
}

TEST(FieldMapTest, GetReturnsNullptrForUnknownTag) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "42";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 0, 2));
  EXPECT_EQ(fm.get(flm::fix::FieldTag::OrderQty), nullptr);
}

TEST(FieldMapTest, SetOverwritesExistingTag) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "99";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 0, 1));
  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 1, 1));

  auto result = fm.get(flm::fix::FieldTag::ClOrdID);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->offset_, 1u);
  EXPECT_EQ(result->length_, 1u);
}

TEST(FieldMapTest, MultipleDistinctTagsAreStoredIndependently) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "1234";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 0, 2));
  fm.set(makeField(flm::fix::FieldTag::OrderQty, 2, 2));

  auto a = fm.get(flm::fix::FieldTag::ClOrdID);
  auto b = fm.get(flm::fix::FieldTag::OrderQty);

  ASSERT_NE(a, nullptr);
  ASSERT_NE(b, nullptr);
  EXPECT_EQ(a->offset_, 0u);
  EXPECT_EQ(b->offset_, 2u);
}

// ---------------------------------------------------------------------------
// FieldMap::getInt / getChar / getLong
// ---------------------------------------------------------------------------

TEST(FieldMapTest, GetIntParsesPositiveInteger) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "12345";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::OrderQty, 0, 5));
  auto val = fm.getInt(flm::fix::FieldTag::OrderQty);

  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 12345);
}

TEST(FieldMapTest, GetIntReturnsNulloptForMissingTag) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "42";
  flm::fix::FieldMap fm(buf, msg);

  EXPECT_FALSE(fm.getInt(flm::fix::FieldTag::OrderQty).has_value());
}

TEST(FieldMapTest, GetLongParsesLargeValue) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "9876543210";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::OrderQty, 0, 10));
  auto val = fm.getLong(flm::fix::FieldTag::OrderQty);

  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 9876543210L);
}

TEST(FieldMapTest, GetLongReturnsNulloptForMissingTag) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "1";
  flm::fix::FieldMap fm(buf, msg);

  EXPECT_FALSE(fm.getLong(flm::fix::FieldTag::ClOrdID).has_value());
}

TEST(FieldMapTest, GetCharReturnsSingleCharacter) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "B";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::Side, 0, 1));
  auto val = fm.getChar(flm::fix::FieldTag::Side);

  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, 'B');
}

TEST(FieldMapTest, GetCharReturnsNulloptForMissingTag) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "X";
  flm::fix::FieldMap fm(buf, msg);

  EXPECT_FALSE(fm.getChar(flm::fix::FieldTag::Side).has_value());
}

TEST(FieldMapTest, GetIntParsesNegativeInteger) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "-77";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::OrderQty, 0, 3));
  auto val = fm.getInt(flm::fix::FieldTag::OrderQty);

  ASSERT_TRUE(val.has_value());
  EXPECT_EQ(*val, -77);
}

// ---------------------------------------------------------------------------
// FieldMap::getFields
// ---------------------------------------------------------------------------

TEST(FieldMapTest, GetTagsEmptyWhenNoFieldsSet) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  flm::fix::FieldMap fm(buf, "");

  EXPECT_TRUE(fm.getFields().empty());
}

TEST(FieldMapTest, GetTagsReflectsSetFields) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "XXYY";
  flm::fix::FieldMap fm(buf, msg);
  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 0, 2));
  fm.set(makeField(flm::fix::FieldTag::OrderQty, 2, 2));

  auto fields = fm.getFields();
  EXPECT_EQ(fields.size(), 2u);

  bool has_clord_id =
      std::find_if(fields.begin(), fields.end(), [](const flm::fix::Field& f) {
        return f.tag_ == flm::fix::FieldTag::ClOrdID;
      }) != fields.end();

  bool has_order_qty =
      std::find_if(fields.begin(), fields.end(), [](const flm::fix::Field& f) {
        return f.tag_ == flm::fix::FieldTag::OrderQty;
      }) != fields.end();

  EXPECT_TRUE(has_clord_id);
  EXPECT_TRUE(has_order_qty);
}

TEST(FieldMapTest, GetTagsDoesNotDuplicateOnOverwrite) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "AB";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 0, 1));
  fm.set(makeField(flm::fix::FieldTag::ClOrdID, 1, 1));  // overwrite

  EXPECT_EQ(fm.getFields().size(), 1u);
}

// ---------------------------------------------------------------------------
// FieldMap::addGroup / getGroup
// ---------------------------------------------------------------------------

TEST(FieldMapTest, GetGroupReturnsNullptrForMissingTag) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  flm::fix::FieldMap fm(buf, "");

  EXPECT_EQ(fm.getGroup(flm::fix::FieldTag::NoLegs, 0), nullptr);
}

TEST(FieldMapTest, AddGroupReturnsNonNull) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  flm::fix::FieldMap fm(buf, "");

  flm::fix::FieldMap* grp = fm.addGroup(flm::fix::FieldTag::NoLegs);
  EXPECT_NE(grp, nullptr);
}

TEST(FieldMapTest, AddGroupMakesTagRetrievable) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  flm::fix::FieldMap fm(buf, "");

  fm.addGroup(flm::fix::FieldTag::NoLegs);
  EXPECT_NE(fm.getGroup(flm::fix::FieldTag::NoLegs, 0), nullptr);
}

TEST(FieldMapTest, MultipleAddGroupsAreIndexedInOrder) {
  flm::fix::Buffer buf(DEFAULT_BUF * 2);
  flm::fix::FieldMap fm(buf, "");

  flm::fix::FieldMap* g0 = fm.addGroup(flm::fix::FieldTag::NoLegs);
  flm::fix::FieldMap* g1 = fm.addGroup(flm::fix::FieldTag::NoLegs);
  flm::fix::FieldMap* g2 = fm.addGroup(flm::fix::FieldTag::NoLegs);

  EXPECT_EQ(fm.getGroup(flm::fix::FieldTag::NoLegs, 0), g0);
  EXPECT_EQ(fm.getGroup(flm::fix::FieldTag::NoLegs, 1), g1);
  EXPECT_EQ(fm.getGroup(flm::fix::FieldTag::NoLegs, 2), g2);
}

TEST(FieldMapTest, GetGroupOutOfRangeReturnsNullptr) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  flm::fix::FieldMap fm(buf, "");

  fm.addGroup(flm::fix::FieldTag::NoLegs);
  EXPECT_EQ(fm.getGroup(flm::fix::FieldTag::NoLegs, 1), nullptr);
}

TEST(FieldMapTest, GetGroupOnNonGroupTagReturnsNullptr) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "42";
  flm::fix::FieldMap fm(buf, msg);

  // set a plain field (no groups_) then try to retrieve it as a group
  fm.set(makeField(flm::fix::FieldTag::OrderQty, 0, 2));
  EXPECT_EQ(fm.getGroup(flm::fix::FieldTag::OrderQty, 0), nullptr);
}

TEST(FieldMapTest, GroupFieldMapCanStoreItsOwnFields) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  const char* msg = "100";
  flm::fix::FieldMap fm(buf, msg);

  flm::fix::FieldMap* grp = fm.addGroup(flm::fix::FieldTag::NoLegs);
  ASSERT_NE(grp, nullptr);

  grp->set(makeField(flm::fix::FieldTag::LegQty, 0, 3));

  auto f = grp->get(flm::fix::FieldTag::LegQty);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->length_, 3u);
}

TEST(FieldMapTest, TwoGroupsHaveIndependentFields) {
  flm::fix::Buffer buf(DEFAULT_BUF * 2);
  const char* msg = "1020";
  flm::fix::FieldMap fm(buf, msg);

  flm::fix::FieldMap* g0 = fm.addGroup(flm::fix::FieldTag::NoLegs);
  flm::fix::FieldMap* g1 = fm.addGroup(flm::fix::FieldTag::NoLegs);
  ASSERT_NE(g0, nullptr);
  ASSERT_NE(g1, nullptr);

  g0->set(makeField(flm::fix::FieldTag::LegQty, 0, 2));
  g1->set(makeField(flm::fix::FieldTag::LegQty, 2, 2));

  EXPECT_EQ(g0->get(flm::fix::FieldTag::LegQty)->offset_, 0u);
  EXPECT_EQ(g1->get(flm::fix::FieldTag::LegQty)->offset_, 2u);
}

TEST(FieldMapTest, AddGroupReturnsNullptrWhenBufferExhausted) {
  flm::fix::Buffer buf(DEFAULT_BUF);
  flm::fix::FieldMap fm(buf, "");

  // Exhaust remaining space
  try {
    buf.allocate(buf.remaining());
  } catch (...) {
  }
  flm::fix::FieldMap* grp = fm.addGroup(flm::fix::FieldTag::NoLegs);
  EXPECT_EQ(grp, nullptr);
}

// ---------------------------------------------------------------------------
// Field::groupCount
// ---------------------------------------------------------------------------

TEST(FieldMapTest, GroupCountReflectsNumberOfAddGroupCalls) {
  flm::fix::Buffer buf(DEFAULT_BUF * 100);
  flm::fix::FieldMap fm(buf, "");

  fm.addGroup(flm::fix::FieldTag::NoLegs);
  fm.addGroup(flm::fix::FieldTag::NoLegs);
  fm.addGroup(flm::fix::FieldTag::NoLegs);

  auto f = fm.get(flm::fix::FieldTag::NoLegs);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->groupCount(), 3u);
}

TEST(FieldMapTest, GroupCountIsZeroForPlainField) {
  flm::fix::Buffer buf(DEFAULT_BUF * 2);
  const char* msg = "1";
  flm::fix::FieldMap fm(buf, msg);

  fm.set(makeField(flm::fix::FieldTag::OrderQty, 0, 1));
  auto f = fm.get(flm::fix::FieldTag::OrderQty);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->groupCount(), 0u);
}

// ---------------------------------------------------------------------------
// FieldMap nested groups (group-of-groups)
// ---------------------------------------------------------------------------

TEST(FieldMapTest, NestedGroupsAreSupported) {
  flm::fix::Buffer buf(DEFAULT_BUF * 100);
  flm::fix::FieldMap fm(buf, "");

  flm::fix::FieldMap* outer = fm.addGroup(flm::fix::FieldTag::NoLegs);
  ASSERT_NE(outer, nullptr);

  flm::fix::FieldMap* inner = outer->addGroup(flm::fix::FieldTag::NoAllocs);
  ASSERT_NE(inner, nullptr);

  EXPECT_EQ(outer->getGroup(flm::fix::FieldTag::NoAllocs, 0), inner);
}