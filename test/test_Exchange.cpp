#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>

#include "Exchange.hpp"

namespace flm = FastLittleMarket;

/*
3 pillars: correctness, race detection, performance

UNIT TESTS

Exchange {
- constructor works
- singleton is enforced
- destructor works
- getShard() works: verify basic string cases
Under one ThreadPool worker:
- addOrder() works: verify (directly on OrderBook) order has been added
- cancelOrder() works: verify (directly on OrderBook) order has been canceled
Under multiple ThreadPool workers:
- addOrder() works: verify order, verify parallelism (2 tests)
- cancelOrder() works: verify order, verify parallelism (2 tests)
}
*/

class CountDownLatch {
 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  int count_;

 public:
  explicit CountDownLatch(int count) : count_(count) {}

  void countDown() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (--count_ == 0) cv_.notify_all();
  }

  void await() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return count_ == 0; });
  }

  template <typename Rep, typename Period>
  bool await(const std::chrono::duration<Rep, Period>& timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.wait_for(lock, timeout, [this] { return count_ == 0; });
  }
};

TEST(ExchangeConcurrency, MultipleAgentsAddOrders) {
  auto& exchange = flm::Exchange::getInstance();

  constexpr int NUM_AGENTS = 10;
  constexpr int ORDERS_PER_AGENT = 100;
  std::vector<std::future<void>> futures;

  CountDownLatch latch(NUM_AGENTS * ORDERS_PER_AGENT);

  // 10 agents → 1000 orders total
  for (int agent = 0; agent < NUM_AGENTS; ++agent) {
    futures.push_back(
        std::async(std::launch::async, [&exchange, agent, &latch]() {
          for (int i = 0; i < ORDERS_PER_AGENT; ++i) {
            exchange.addOrder("AAPL",
                              flm::Order(agent * 1000 + i, flm::OrderSide::Buy,
                                         100.0 + i * 0.01, 10, "agent"));
            latch.countDown();
          }
        }));
  }

  latch.await(std::chrono::seconds(10));  // Fail if not all complete
  SUCCEED();
}