#include <gtest/gtest.h>

#include "Field.hpp"
#include "FieldList.hpp"
#include "FieldTag.hpp"

namespace flm = fast_little_market;

static flm::fix::Field makeField(flm::fix::FieldTag tag, uint16_t offset,
                                 uint16_t length) {
  return flm::fix::Field(tag, offset, length);
}

// ---------------------------------------------------------------------------
// put
// ---------------------------------------------------------------------------

TEST(FieldListTest, PutStoresField) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  EXPECT_EQ(list.size(), 1u);
}

TEST(FieldListTest, PutMultipleFieldsIncreasesSize) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  list.put(makeField(flm::fix::FieldTag::OrderQty, 4, 2));
  list.put(makeField(flm::fix::FieldTag::OrdType, 6, 1));
  EXPECT_EQ(list.size(), 3u);
}

TEST(FieldListTest, PutDuplicateTagOverwritesExisting) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 10, 6));
  EXPECT_EQ(list.size(), 1u);
  auto* f = list.find(flm::fix::FieldTag::ClOrdID);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->offset_, 10);
  EXPECT_EQ(f->length_, 6);
}

TEST(FieldListTest, PutReturnsMutableReference) {
  flm::fix::FieldList list;
  auto& ref = list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  ref.offset_ = 99;
  auto* f = list.find(flm::fix::FieldTag::ClOrdID);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->offset_, 99);
}

TEST(FieldListTest, PutAtCapacityAsserts) {
  flm::fix::FieldList list;
  // Fill up to capacity using distinct tags — this relies on having enough tags
  // If FIX_MSG_MAX_FIELD_COUNT changes, this test may need updating
  EXPECT_DEATH(
      {
        for (int i = 0; i < (int)flm::fix::FIX_MSG_MAX_FIELD_COUNT + 1; i++) {
          list.put(
              flm::fix::Field(static_cast<flm::fix::FieldTag>(i + 1), 0, 0));
        }
      },
      "FieldList capacity exceeded");
}

// ---------------------------------------------------------------------------
// find
// ---------------------------------------------------------------------------

TEST(FieldListTest, FindReturnsNullptrWhenEmpty) {
  flm::fix::FieldList list;
  EXPECT_EQ(list.find(flm::fix::FieldTag::ClOrdID), nullptr);
}

TEST(FieldListTest, FindReturnsNullptrForMissingTag) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  EXPECT_EQ(list.find(flm::fix::FieldTag::OrderQty), nullptr);
}

TEST(FieldListTest, FindReturnsCorrectField) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 5, 3));
  auto* f = list.find(flm::fix::FieldTag::ClOrdID);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->tag_, flm::fix::FieldTag::ClOrdID);
  EXPECT_EQ(f->offset_, 5);
  EXPECT_EQ(f->length_, 3);
}

TEST(FieldListTest, FindMutableAllowsWrite) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  auto* f = list.find(flm::fix::FieldTag::ClOrdID);
  ASSERT_NE(f, nullptr);
  f->offset_ = 42;
  EXPECT_EQ(list.find(flm::fix::FieldTag::ClOrdID)->offset_, 42);
}

TEST(FieldListTest, FindConstReturnsCorrectField) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::OrderQty, 2, 5));
  const flm::fix::FieldList& clist = list;
  const auto* f = clist.find(flm::fix::FieldTag::OrderQty);
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->offset_, 2);
}

TEST(FieldListTest, FindConstReturnsNullptrForMissingTag) {
  flm::fix::FieldList list;
  const flm::fix::FieldList& clist = list;
  EXPECT_EQ(clist.find(flm::fix::FieldTag::ClOrdID), nullptr);
}

// ---------------------------------------------------------------------------
// contains
// ---------------------------------------------------------------------------

TEST(FieldListTest, ContainsReturnsFalseWhenEmpty) {
  flm::fix::FieldList list;
  EXPECT_FALSE(list.contains(flm::fix::FieldTag::ClOrdID));
}

TEST(FieldListTest, ContainsReturnsTrueForStoredTag) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  EXPECT_TRUE(list.contains(flm::fix::FieldTag::ClOrdID));
}

TEST(FieldListTest, ContainsReturnsFalseForAbsentTag) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 4));
  EXPECT_FALSE(list.contains(flm::fix::FieldTag::OrderQty));
}

// ---------------------------------------------------------------------------
// fields / size
// ---------------------------------------------------------------------------

TEST(FieldListTest, SizeIsZeroOnConstruction) {
  flm::fix::FieldList list;
  EXPECT_EQ(list.size(), 0u);
}

TEST(FieldListTest, FieldsSpanIsEmptyOnConstruction) {
  flm::fix::FieldList list;
  EXPECT_EQ(list.fields().size(), 0u);
}

TEST(FieldListTest, FieldsSpanReflectsStoredFields) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 2));
  list.put(makeField(flm::fix::FieldTag::OrderQty, 2, 3));
  auto span = list.fields();
  EXPECT_EQ(span.size(), 2u);
  bool has_clord_id =
      std::find_if(span.begin(), span.end(), [](const flm::fix::Field& f) {
        return f.tag_ == flm::fix::FieldTag::ClOrdID;
      }) != span.end();
  bool has_order_qty =
      std::find_if(span.begin(), span.end(), [](const flm::fix::Field& f) {
        return f.tag_ == flm::fix::FieldTag::OrderQty;
      }) != span.end();
  EXPECT_TRUE(has_clord_id);
  EXPECT_TRUE(has_order_qty);
}

TEST(FieldListTest, FieldsSpanDoesNotIncludeUnusedSlots) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 2));
  EXPECT_EQ(list.fields().size(), 1u);
}

TEST(FieldListTest, SizeDoesNotGrowOnDuplicatePut) {
  flm::fix::FieldList list;
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 0, 2));
  list.put(makeField(flm::fix::FieldTag::ClOrdID, 5, 3));
  EXPECT_EQ(list.size(), 1u);
}