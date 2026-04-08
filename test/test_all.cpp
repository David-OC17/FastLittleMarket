#include <gtest/gtest.h>

#include "test_Order.cpp"
#include "test_OrderBook.cpp"
#include "test_PriorityQueueAdapter.cpp"
#include "test_ThreadPool.cpp"
#include "test_Exchange.cpp"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}