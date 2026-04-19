#include <gtest/gtest.h>

#include "ThreadPool.hpp"

namespace flm = fast_little_market;

class ThreadPoolTest : public ::testing::Test {
 protected:
  std::atomic<int> task_counter_{0};
};

TEST_F(ThreadPoolTest, ConstructorCreatesWorkers) {
  flm::ThreadPool pool;
  std::this_thread::sleep_for(
      std::chrono::milliseconds(10));  // Give workers time to start
  SUCCEED();
}

TEST_F(ThreadPoolTest, DestructorStopsWorkers) {
  { flm::ThreadPool pool; }
  SUCCEED();
}

TEST_F(ThreadPoolTest, SingleWorkerExecutesTask) {
  flm::ThreadPool pool;
  task_counter_ = 0;

  pool.enqueue([&]() { task_counter_ = 42; }, 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(42, task_counter_);
}

TEST_F(ThreadPoolTest, SingleWorkerSleepsAndExecutes) {
  flm::ThreadPool pool;
  task_counter_ = 0;

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  pool.enqueue([&]() { task_counter_ = 123; }, 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(123, task_counter_);
}

TEST_F(ThreadPoolTest, SingleWorkerMultipleTasks) {
  flm::ThreadPool pool;
  std::atomic<int> tasks_completed{0};

  auto task = [&tasks_completed]() { tasks_completed++; };

  pool.enqueue(task, 0);
  pool.enqueue(task, 0);
  pool.enqueue(task, 0);
  pool.enqueue(task, 0);
  pool.enqueue(task, 0);

  // Wait until all 5 tasks complete
  auto start = std::chrono::steady_clock::now();
  while (tasks_completed != 5 &&
         std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  EXPECT_EQ(5, tasks_completed);
}