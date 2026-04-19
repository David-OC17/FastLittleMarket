#include "ExchangeLogger.hpp"

namespace fast_little_market {

ExchangeLogger& ExchangeLogger::getInstance() {
  static ExchangeLogger instance;
  return instance;
}

quill::Logger* ExchangeLogger::getGlobalLogger() const {
  return global_logger_;
}

quill::Logger* global_logger = ExchangeLogger::getInstance().getGlobalLogger();

}  // namespace fast_little_market