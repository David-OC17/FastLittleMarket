#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "ExchangeLoggerMacros.hpp"
#include "Order.hpp"

namespace flm = FastLittleMarket;

static std::string read_log(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Failed to open file: " + path);
  }

  f.seekg(0, std::ios::end);
  std::string content;
  content.resize(f.tellg());
  f.seekg(0, std::ios::beg);
  f.read(&content[0], content.size());

  return content;
}

static void dump_log(const std::string& log, const std::string& test_name) {
  std::cout << "\n=== [" << test_name << "] log contents ===\n"
            << (log.empty() ? "<EMPTY>" : log) << "\n=== end ===\n";
  std::cout.flush();
}

static bool log_contains(const std::string& log, const std::string& needle) {
  return log.find(needle) != std::string::npos;
}

class ExchangeLoggerTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() { ASSERT_NE(flm::global_logger, nullptr); }
  flm::GlobalSequencer sequencer_;  // lives for the duration of each test

  flm::Order make_order(int id, uint32_t price_q4, uint32_t vol, uint8_t side) {
    return flm::Order(id, price_q4, vol, side == flm::BUY_SIDE, "JPMORG",
                      sequencer_);
  }

  std::string find_log_file() { return flm::QUILL_LOG_FILE; }
};

TEST_F(ExchangeLoggerTest, LogInfo) {
  LOG_INFO("test_info_message_unique_42");

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());

  EXPECT_TRUE(log_contains(log, "test_info_message_unique_42"))
      << "INFO message not found in log";
  EXPECT_TRUE(log_contains(log, "LOG_INFO")) << "Log level tag missing";
}

TEST_F(ExchangeLoggerTest, LogWarning) {
  LOG_WARNING("test_warning_message_unique_43");

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());

  EXPECT_TRUE(log_contains(log, "test_warning_message_unique_43"))
      << "WARNING message not found in log";
  EXPECT_TRUE(log_contains(log, "LOG_WARNING")) << "Log level tag missing";
}

TEST_F(ExchangeLoggerTest, LogError) {
  LOG_ERROR("test_error_message_unique_44");

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());

  EXPECT_TRUE(log_contains(log, "test_error_message_unique_44"))
      << "ERROR message not found in log";
  EXPECT_TRUE(log_contains(log, "LOG_ERROR")) << "Log level tag missing";
}

TEST_F(ExchangeLoggerTest, MultipleThreadsWriteToSameLog) {
  constexpr int NUM_THREADS = 4;
  constexpr int WRITES_PER_THREAD = 100;

  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);

  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([t]() {
      for (int i = 0; i < WRITES_PER_THREAD; ++i) {
        LOG_INFO("thread_{}_entry_{}", t, i);
      }
    });
  }
  for (auto& th : threads) th.join();

  // Give the async backend extra time to drain under contention
  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());

  // Every thread's first and last entry must appear
  for (int t = 0; t < NUM_THREADS; ++t) {
    EXPECT_TRUE(log_contains(log, "thread_" + std::to_string(t) + "_entry_0"))
        << "Missing first entry from thread " << t;
    EXPECT_TRUE(log_contains(log, "thread_" + std::to_string(t) + "_entry_" +
                                      std::to_string(WRITES_PER_THREAD - 1)))
        << "Missing last entry from thread " << t;
  }

  // No line should be garbled — every line must start with a time token
  // (quill guarantees atomic line writes, so partial lines are not possible)
  std::istringstream stream(log);
  std::string line;
  int malformed = 0;
  while (std::getline(stream, line)) {
    if (line.empty()) continue;
    // Pattern format starts with HH:MM:SS
    bool starts_with_time =
        (line.size() > 8 && line[2] == ':' && line[5] == ':');
    if (!starts_with_time) ++malformed;
  }
  EXPECT_EQ(malformed, 0) << "Found " << malformed << " malformed log lines";
}

TEST_F(ExchangeLoggerTest, MacroNewOrderSingle) {
  auto order = make_order(1, 1000000, 100, flm::BUY_SIDE);
  LOG_NEW_ORDER_SINGLE(order);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "NewOrderSingle"));
  EXPECT_TRUE(log_contains(log, "id=1"));
  EXPECT_TRUE(log_contains(log, "BUY"));
  EXPECT_TRUE(log_contains(log, "price_q4=1000000"));
  EXPECT_TRUE(log_contains(log, "vol=100"));
  EXPECT_TRUE(log_contains(log, "client=JPMORG"));
}

TEST_F(ExchangeLoggerTest, MacroCancelOrderRequest) {
  auto order = make_order(2, 1000000, 50, flm::SELL_SIDE);
  LOG_CANCEL_ORDER_REQUEST(order);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "CancelOrderRequest"));
  EXPECT_TRUE(log_contains(log, "id=2"));
  EXPECT_TRUE(log_contains(log, "SELL"));
  EXPECT_TRUE(log_contains(log, "vol=50"));
}

TEST_F(ExchangeLoggerTest, MacroMatch) {
  auto passive = make_order(3, 1000000, 100, flm::BUY_SIDE);
  auto aggressive = make_order(4, 990000, 40, flm::SELL_SIDE);
  LOG_MATCH(passive, aggressive, 40, 1000000);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "Match"));
  EXPECT_TRUE(log_contains(log, "passive_id=3"));
  EXPECT_TRUE(log_contains(log, "aggressive_id=4"));
  EXPECT_TRUE(log_contains(log, "fill_vol=40"));
  EXPECT_TRUE(log_contains(log, "fill_price_q4=1000000"));
}

TEST_F(ExchangeLoggerTest, MacroOrderAccepted) {
  auto order = make_order(5, 1050000, 200, flm::BUY_SIDE);
  LOG_ORDER_ACCEPTED(order);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "OrderAccepted"));
  EXPECT_TRUE(log_contains(log, "id=5"));
  EXPECT_TRUE(log_contains(log, "vol=200"));
}

TEST_F(ExchangeLoggerTest, MacroOrderRejected) {
  auto order = make_order(6, 0, 0, flm::BUY_SIDE);  // invalid order
  LOG_ORDER_REJECTED(order, "InvalidPrice");

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "OrderRejected"));
  EXPECT_TRUE(log_contains(log, "id=6"));
  EXPECT_TRUE(log_contains(log, "reason=InvalidPrice"));
}

TEST_F(ExchangeLoggerTest, MacroOrderReplaced) {
  auto old_order = make_order(7, 1000000, 100, flm::BUY_SIDE);
  auto new_order = make_order(8, 1010000, 80, flm::BUY_SIDE);
  LOG_ORDER_REPLACED(old_order, new_order);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "OrderReplaced"));
  EXPECT_TRUE(log_contains(log, "old_id=7"));
  EXPECT_TRUE(log_contains(log, "new_id=8"));
  EXPECT_TRUE(log_contains(log, "old_price_q4=1000000"));
  EXPECT_TRUE(log_contains(log, "new_price_q4=1010000"));
  EXPECT_TRUE(log_contains(log, "old_vol=100"));
  EXPECT_TRUE(log_contains(log, "new_vol=80"));
}

TEST_F(ExchangeLoggerTest, MacroOrderCanceled) {
  auto order = make_order(9, 1000000, 75, flm::SELL_SIDE);
  LOG_ORDER_CANCELED(order);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "OrderCanceled"));
  EXPECT_TRUE(log_contains(log, "id=9"));
  EXPECT_TRUE(log_contains(log, "SELL"));
  EXPECT_TRUE(log_contains(log, "vol=75"));
}

TEST_F(ExchangeLoggerTest, MacroOrderExecuted) {
  auto order = make_order(10, 1000000, 100, flm::BUY_SIDE);
  LOG_ORDER_EXECUTED(order, 60, 40, 1000000);

  flm::global_logger->flush_log();
  std::string log = read_log(find_log_file());
  EXPECT_TRUE(log_contains(log, "OrderExecuted"));
  EXPECT_TRUE(log_contains(log, "id=10"));
  EXPECT_TRUE(log_contains(log, "fill_vol=60"));
  EXPECT_TRUE(log_contains(log, "leaves_vol=40"));
  EXPECT_TRUE(log_contains(log, "fill_price_q4=1000000"));
}

// ── Full lifecycle (single-threaded simulation)
// ───────────────────────────────

// TEST_F(ExchangeLoggerTest, SimulatedFullLifecycle) {
//

//   auto bid = make_order(20, 1000000, 100, flm::BUY_SIDE);
//   auto ask = make_order(21, 990000, 60, flm::SELL_SIDE);

//   // Inbound messages
//   LOG_NEW_ORDER_SINGLE(bid);
//   LOG_NEW_ORDER_SINGLE(ask);

//   // Engine accepts both
//   LOG_ORDER_ACCEPTED(bid);
//   LOG_ORDER_ACCEPTED(bid);

//   // Prices cross — partial fill: ask fully consumed, bid has 40 remaining
//   logger.log_match(bid, ask, 60, 1000000);
//   logger.log_executed(ask, 60, 0, 1000000);   // ask fully filled
//   logger.log_executed(bid, 60, 40, 1000000);  // bid partially filled

//   // Client cancels the remaining bid
//   logger.log_cancel_request(bid);
//   logger.log_canceled(bid);

//   std::string log = read_log(find_log_file(),
//   std::chrono::milliseconds(200));

//   // Verify the full sequence appears — order matters for an audit log
//   auto pos = [&](const std::string& needle) { return log.find(needle); };

//   EXPECT_NE(pos("NewOrderSingle"), std::string::npos);
//   EXPECT_NE(pos("OrderAccepted"), std::string::npos);
//   EXPECT_NE(pos("Match"), std::string::npos);
//   EXPECT_NE(pos("OrderExecuted"), std::string::npos);
//   EXPECT_NE(pos("CancelOrderRequest"), std::string::npos);
//   EXPECT_NE(pos("OrderCanceled"), std::string::npos);

//   // Causal ordering: NewOrderSingle must precede Match
//   EXPECT_LT(pos("NewOrderSingle"), pos("Match"))
//       << "NewOrderSingle must appear before Match in the log";

//   // Match must precede OrderExecuted
//   EXPECT_LT(pos("Match"), pos("OrderExecuted"))
//       << "Match must appear before OrderExecuted in the log";

//   // OrderExecuted must precede CancelOrderRequest
//   EXPECT_LT(pos("OrderExecuted"), pos("CancelOrderRequest"))
//       << "OrderExecuted must appear before CancelOrderRequest in the log";
// }