#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#include "GlobalSequencer.hpp"

namespace flm = fast_little_market;

class GlobalSequencerTest : public ::testing::Test {
 protected:
  flm::GlobalSequencer sequencer_;
};

TEST_F(GlobalSequencerTest, ConstructorInitializesNonZeroEpoch) {
  flm::GlobalSequencer seq;
  EXPECT_GT(seq.next_timestamp_ns(), 0ULL);
}

TEST_F(GlobalSequencerTest, GeneratesMonotonicTimestamps) {
  uint64_t t1 = sequencer_.next_timestamp_ns();
  uint64_t t2 = sequencer_.next_timestamp_ns();
  uint64_t t3 = sequencer_.next_timestamp_ns();

  // Consecutive calls increment by exactly 1 — delta is what matters,
  // not absolute value (epoch_ns is an arbitrary nanosecond timestamp)
  EXPECT_EQ(t2 - t1, 1ULL);
  EXPECT_EQ(t3 - t2, 1ULL);
}

TEST_F(GlobalSequencerTest, SequentialCallsIncrementByOne) {
  uint64_t prev = sequencer_.next_timestamp_ns();
  for (int i = 0; i < 1000; ++i) {
    uint64_t curr = sequencer_.next_timestamp_ns();
    EXPECT_EQ(curr - prev, 1ULL);
    prev = curr;
  }
}

TEST_F(GlobalSequencerTest, ThreadSafety_MultiThreaded) {
  constexpr int NUM_THREADS = 8;
  constexpr int CALLS_PER_THREAD = 10000;

  // Each thread writes to its own vector — no shared mutable state
  std::vector<std::vector<uint64_t>> per_thread(NUM_THREADS);
  for (auto& v : per_thread) v.reserve(CALLS_PER_THREAD);

  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);

  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < CALLS_PER_THREAD; ++i)
        per_thread[t].push_back(sequencer_.next_timestamp_ns());
    });
  }
  for (auto& thread : threads) thread.join();

  // Merge all results
  std::vector<uint64_t> all;
  all.reserve(NUM_THREADS * CALLS_PER_THREAD);
  for (auto& v : per_thread) all.insert(all.end(), v.begin(), v.end());

  // After sorting, every value must be strictly unique
  // (seq_counter is fetch_add — each call gets a distinct value)
  std::sort(all.begin(), all.end());
  for (size_t i = 1; i < all.size(); ++i) {
    EXPECT_LT(all[i - 1], all[i])
        << "Duplicate at index " << i << ": " << all[i];
  }
}

TEST_F(GlobalSequencerTest, PerformanceSingleThread) {
  constexpr int CALLS = 1'000'000;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < CALLS; ++i) sequencer_.next_timestamp_ns();
  auto end = std::chrono::high_resolution_clock::now();

  double ns_per_call =
      static_cast<double>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
              .count()) /
      CALLS;

  // Informational — relaxed atomic fetch_add typically costs 2-10ns.
  // This threshold is deliberately loose to avoid CI flakiness.
  EXPECT_LT(ns_per_call, 200.0) << "Average: " << ns_per_call << " ns/call";
}

TEST_F(GlobalSequencerTest, MultipleInstancesIndependent) {
  flm::GlobalSequencer seq1;
  flm::GlobalSequencer seq2;

  uint64_t t1_a = seq1.next_timestamp_ns();
  uint64_t t2_a = seq2.next_timestamp_ns();
  uint64_t t1_b = seq1.next_timestamp_ns();
  uint64_t t2_b = seq2.next_timestamp_ns();

  // Each instance has its own seq_counter starting at 0 and its own epoch_ns.
  // Consecutive calls on the same instance always differ by exactly 1.
  EXPECT_EQ(t1_b - t1_a, 1ULL);
  EXPECT_EQ(t2_b - t2_a, 1ULL);
}

TEST_F(GlobalSequencerTest, SequentialAfterThreadStress) {
  std::vector<std::thread> threads(4);
  for (auto& t : threads) {
    t = std::thread([this]() {
      for (int i = 0; i < 10000; ++i) sequencer_.next_timestamp_ns();
    });
  }
  for (auto& t : threads) t.join();

  // After concurrent stress, sequential increments must still be exactly 1
  uint64_t a = sequencer_.next_timestamp_ns();
  uint64_t b = sequencer_.next_timestamp_ns();
  EXPECT_EQ(b - a, 1ULL);
}