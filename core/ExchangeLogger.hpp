#pragma once

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/FileSink.h"
#define QUILL_DISABLE_NON_PREFIXED_MACROS
#include "Order.hpp"
#include "quill/Logger.h"

namespace FastLittleMarket {

#ifndef FLM_LOG_DIR
#define FLM_LOG_DIR "."
#endif

static constexpr char QUILL_LOG_FILE[] = FLM_LOG_DIR "/exchange.log";

class ExchangeLogger {
 private:
  quill::Logger* global_logger_;

  static quill::Logger* make_logger(const char* name,
                                    std::shared_ptr<quill::Sink> sink) {
    return quill::Frontend::create_or_get_logger(
        name, std::move(sink),
        quill::PatternFormatterOptions{
            "%(time) [%(thread_id)] %(short_source_location:<28) "
            "LOG_%(log_level:<9) %(logger:<12) %(message)",
            "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
  }

  ExchangeLogger() {
    quill::Backend::start();

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        QUILL_LOG_FILE,
        []() {
          quill::FileSinkConfig cfg;
          cfg.set_open_mode('w');
#ifndef FLM_TESTING
          cfg.set_filename_append_option(
              quill::FilenameAppendOption::StartDateTime);
#endif
          return cfg;
        }(),
        quill::FileEventNotifier{});

    global_logger_ = make_logger("global", file_sink);
  }

 public:
  static ExchangeLogger& getInstance();

  quill::Logger* getGlobalLogger() const;

  ~ExchangeLogger() { quill::Backend::stop(); }
};

extern quill::Logger* global_logger;

}  // namespace FastLittleMarket