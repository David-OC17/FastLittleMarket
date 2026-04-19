#include <gtest/gtest.h>

#include "FixAlloc.hpp"

namespace flm = fast_little_market;

TEST(BufferTest, ConstructorSetsCapacityAndZeroAllocated) {
  flm::fix::Buffer buf(256);
  EXPECT_EQ(buf.capacity_, 256);
  EXPECT_EQ(buf.allocated(), 0);
  EXPECT_EQ(buf.remaining(), 256);
}

TEST(BufferTest, ZeroCapacityConstructor) {
  flm::fix::Buffer buf(0);
  EXPECT_EQ(buf.capacity_, 0);
  EXPECT_EQ(buf.allocated(), 0);
  EXPECT_EQ(buf.remaining(), 0);
}

TEST(BufferTest, CopyConstructorIsDeleted) {
  EXPECT_FALSE(std::is_copy_constructible_v<flm::fix::Buffer>);
}

TEST(BufferTest, CopyAssignmentIsDeleted) {
  EXPECT_FALSE(std::is_copy_assignable_v<flm::fix::Buffer>);
}

TEST(BufferTest, MoveConstructorTransfersOwnership) {
  flm::fix::Buffer a(128);
  a.allocate(32);

  flm::fix::Buffer b(std::move(a));

  EXPECT_EQ(b.capacity_, 128);
  EXPECT_EQ(b.allocated(), 32);
  EXPECT_EQ(b.remaining(), 96);
}

TEST(BufferTest, AllocateReturnsValidPointer) {
  flm::fix::Buffer buf(128);
  auto res = buf.allocate(64);
  ASSERT_TRUE(res.has_value());
  EXPECT_NE(*res, nullptr);
}

TEST(BufferTest, AllocateUpdatesCounters) {
  flm::fix::Buffer buf(128);
  buf.allocate(40);
  EXPECT_EQ(buf.allocated(), 40);
  EXPECT_EQ(buf.remaining(), 88);
}

TEST(BufferTest, AllocateReturnedPointersAreContiguous) {
  flm::fix::Buffer buf(128);
  auto r1 = buf.allocate(32);
  auto r2 = buf.allocate(32);
  ASSERT_TRUE(r1 && r2);
  // second allocation must start exactly 32 bytes after the first
  EXPECT_EQ(static_cast<std::byte*>(*r2), static_cast<std::byte*>(*r1) + 32);
}

TEST(BufferTest, AllocateFailsWhenFull) {
  flm::fix::Buffer buf(64);
  buf.allocate(64);
  auto res = buf.allocate(1);
  ASSERT_FALSE(res.has_value());
  EXPECT_EQ(res.error(), flm::fix::AllocError::out_of_memory);
}

TEST(BufferTest, AllocateFailsOnOverflow) {
  flm::fix::Buffer buf(64);
  auto res = buf.allocate(65);
  ASSERT_FALSE(res.has_value());
  EXPECT_EQ(res.error(), flm::fix::AllocError::out_of_memory);
}

TEST(BufferTest, AllocateEntireCapacitySucceeds) {
  flm::fix::Buffer buf(64);
  auto res = buf.allocate(64);
  EXPECT_TRUE(res.has_value());
  EXPECT_EQ(buf.remaining(), 0);
}

TEST(BufferTest, DeallocateUpdatesCounters) {
  flm::fix::Buffer buf(128);
  buf.allocate(64);
  auto res = buf.deallocate(32);
  ASSERT_TRUE(res.has_value());
  EXPECT_EQ(buf.allocated(), 32);
  EXPECT_EQ(buf.remaining(), 96);
}

TEST(BufferTest, DeallocateAllowsReuse) {
  flm::fix::Buffer buf(64);
  buf.allocate(64);
  buf.deallocate(64);
  auto res = buf.allocate(64);
  EXPECT_TRUE(res.has_value());
}

TEST(BufferTest, DeallocateUnderflowReturnsError) {
  flm::fix::Buffer buf(128);
  buf.allocate(32);
  auto res = buf.deallocate(64);  // more than allocated
  ASSERT_FALSE(res.has_value());
  EXPECT_EQ(res.error(), flm::fix::AllocError::underflow);
}

TEST(BufferTest, DeallocateOnEmptyBufferReturnsError) {
  flm::fix::Buffer buf(128);
  auto res = buf.deallocate(1);
  ASSERT_FALSE(res.has_value());
  EXPECT_EQ(res.error(), flm::fix::AllocError::underflow);
}

TEST(BufferTest, ResetRestoresFullCapacity) {
  flm::fix::Buffer buf(128);
  buf.allocate(100);
  buf.reset();
  EXPECT_EQ(buf.allocated(), 0);
  EXPECT_EQ(buf.remaining(), 128);
}

TEST(BufferTest, ResetAllowsFullReallocation) {
  flm::fix::Buffer buf(64);
  buf.allocate(64);
  buf.reset();
  auto res = buf.allocate(64);
  EXPECT_TRUE(res.has_value());
}

TEST(BufferTest, RemainingAndAllocatedSumToCapacity) {
  flm::fix::Buffer buf(256);
  buf.allocate(100);
  EXPECT_EQ(buf.allocated() + buf.remaining(), buf.capacity_);
}

TEST(AllocatorTest, AllocateReturnsTypedPointer) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int> alloc(buf);
  auto res = alloc.allocate(4);
  ASSERT_TRUE(res.has_value());
  EXPECT_NE(*res, nullptr);
}

TEST(AllocatorTest, AllocateSizesAreScaledByType) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int32_t> alloc(buf);  // sizeof(int32_t) == 4
  alloc.allocate(8);                        // should consume 32 bytes
  EXPECT_EQ(buf.allocated(), 32);
}

TEST(AllocatorTest, AllocateFailsWhenBufferFull) {
  flm::fix::Buffer buf(16);
  flm::fix::Allocator<int32_t> alloc(buf);
  alloc.allocate(4);             // consumes 16 bytes
  auto res = alloc.allocate(1);  // no room for even 1 int
  ASSERT_FALSE(res.has_value());
  EXPECT_EQ(res.error(), flm::fix::AllocError::out_of_memory);
}

TEST(AllocatorTest, AllocatedMemoryIsWritable) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int> alloc(buf);
  auto res = alloc.allocate(4);
  ASSERT_TRUE(res.has_value());
  int* ptr = *res;
  for (int i = 0; i < 4; ++i) ptr[i] = i * 10;
  for (int i = 0; i < 4; ++i) EXPECT_EQ(ptr[i], i * 10);
}

TEST(AllocatorTest, DeallocateScalesByType) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int32_t> alloc(buf);
  auto res = alloc.allocate(4);  // 16 bytes
  ASSERT_TRUE(res.has_value());
  auto dealloc = alloc.deallocate(*res, 4);
  ASSERT_TRUE(dealloc.has_value());
  EXPECT_EQ(buf.allocated(), 0);
}

TEST(AllocatorTest, DeallocateUnderflowReturnsError) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int32_t> alloc(buf);
  auto res = alloc.allocate(2);  // 8 bytes
  ASSERT_TRUE(res.has_value());
  auto dealloc = alloc.deallocate(*res, 4);  // tries to free 16 bytes
  ASSERT_FALSE(dealloc.has_value());
  EXPECT_EQ(dealloc.error(), flm::fix::AllocError::underflow);
}

TEST(AllocatorTest, BufferGetterReturnsSameBuffer) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int> alloc(buf);
  EXPECT_EQ(&alloc.buffer(), &buf);
}

TEST(AllocatorTest, CopySharesBuffer) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int> a1(buf);
  flm::fix::Allocator<int> a2(a1);
  a1.allocate(4);
  // both allocators draw from the same buffer
  EXPECT_EQ(&a1.buffer(), &a2.buffer());
  EXPECT_EQ(buf.allocated(), 4 * sizeof(int));
}

TEST(AllocatorTest, MultipleTypesShareBuffer) {
  flm::fix::Buffer buf(256);
  flm::fix::Allocator<int32_t> int_alloc(buf);
  flm::fix::Allocator<double> dbl_alloc(buf);
  int_alloc.allocate(2);  // 8 bytes
  dbl_alloc.allocate(2);  // 16 bytes
  EXPECT_EQ(buf.allocated(), 24);
}