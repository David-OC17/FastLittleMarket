#include "ExchangeLogger.hpp"

namespace FastLittleMarket {

ExchangeLogger& ExchangeLogger::getInstance() {
  static ExchangeLogger instance;
  return instance;
}

quill::Logger* ExchangeLogger::getGlobalLogger() const {
  return global_logger_;
}

quill::Logger* global_logger = ExchangeLogger::getInstance().getGlobalLogger();

}  // namespace FastLittleMarket