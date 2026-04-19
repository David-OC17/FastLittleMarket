#pragma once

#include <atomic>
#include <cstdint>

#ifdef __linux__
#include <time.h>
#elif _WIN32
#include <windows.h>
#endif

namespace fast_little_market {

inline uint64_t os_nano_time() noexcept {
#ifdef __linux__
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec * 1'000'000'000ULL + ts.tv_nsec;
#elif _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  return (ft.dwHighDateTime * 1'000'000'000ULL + ft.dwLowDateTime * 100 +
          ft.dwLowDateTime % 100 * 10);
#endif
}

class GlobalSequencer {
 private:
  uint64_t epoch_ns;
  std::atomic<uint64_t> seq_counter{0};

 public:
  GlobalSequencer() : epoch_ns(os_nano_time()) {}

  uint64_t next_timestamp_ns() noexcept {
    uint64_t seq = seq_counter.fetch_add(1, std::memory_order_relaxed);
    return epoch_ns + seq;
  }
};

}  // namespace fast_little_market