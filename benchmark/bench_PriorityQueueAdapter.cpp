#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

#include "OrderBook.hpp"

namespace flm = fast_little_market;

// pop, push, empty, find, top, remove

static void BM_PriorityQueueAdapterPushPop(benchmark::State& state) {
  flm::PriorityQueueAdapter<flm::BuyOrderComparator> pq;
  flm::GlobalSequencer sequencer_;

  for (auto _ : state) {
    pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));
    benchmark::ClobberMemory();
    pq.pop();
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_PriorityQueueAdapterPushPop);

static void BM_PriorityQueueAdapterFind(benchmark::State& state) {
  flm::PriorityQueueAdapter<flm::BuyOrderComparator> pq;
  flm::GlobalSequencer sequencer_;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));

  for (auto _ : state) {
    auto found = pq.find(pq.top()->id_ns_);
    benchmark::DoNotOptimize(found);
  }
}
BENCHMARK(BM_PriorityQueueAdapterFind);

static void BM_PriorityQueueAdapterEmpty(benchmark::State& state) {
  flm::PriorityQueueAdapter<flm::BuyOrderComparator> pq;

  for (auto _ : state) {
    benchmark::DoNotOptimize(pq.empty());
  }
}
BENCHMARK(BM_PriorityQueueAdapterEmpty);

static void BM_PriorityQueueAdapterTop(benchmark::State& state) {
  flm::PriorityQueueAdapter<flm::BuyOrderComparator> pq;
  flm::GlobalSequencer sequencer_;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));

  for (auto _ : state) {
    auto top = pq.top();
    benchmark::DoNotOptimize(top);
  }
}
BENCHMARK(BM_PriorityQueueAdapterTop);

static void BM_PriorityQueueAdapterRemove(benchmark::State& state) {
  flm::PriorityQueueAdapter<flm::BuyOrderComparator> pq;
  flm::GlobalSequencer sequencer_;

  pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));

  for (auto _ : state) {
    // EXPECT_TRUE(pq.remove(pq.top()->id_ns_));
    benchmark::ClobberMemory();
    pq.push(flm::Order(1, 1000000, 10, flm::BUY_SIDE, "c1", sequencer_));
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_PriorityQueueAdapterRemove);