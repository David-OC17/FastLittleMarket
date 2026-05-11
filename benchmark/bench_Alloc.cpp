#include <benchmark/benchmark.h>

#include "Alloc.hpp"

namespace flm = fast_little_market;

static void BM_BufferAllocDealloc(benchmark::State& state) {
  flm::fix::Buffer buf(1024);
  for (auto _ : state) {
    void* res = buf.allocate(64);
    benchmark::DoNotOptimize(res);
    buf.deallocate(64);
  }
}
BENCHMARK(BM_BufferAllocDealloc);

static void BM_BufferReset(benchmark::State& state) {
  flm::fix::Buffer buf(1024);
  for (auto _ : state) {
    buf.allocate(512);
    buf.reset();
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_BufferReset);

static void BM_BufferCreateDestroy(benchmark::State& state) {
  for (auto _ : state) {
    flm::fix::Buffer buf(1024);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_BufferCreateDestroy);

// static void BM_BufferAllocateUntilFull(benchmark::State& state) {
//   flm::fix::Buffer buf(1024);
//   for (auto _ : state) {
//     size_t total_allocated = 0;
//     while (total_allocated + 64 <= buf.capacity_) {
//       void* res = buf.allocate(64);
//       benchmark::DoNotOptimize(res);
//       total_allocated += 64;
//     }
//     buf.reset();
//   }
// }
// BENCHMARK(BM_BufferAllocateUntilFull);