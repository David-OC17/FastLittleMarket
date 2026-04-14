#pragma once

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/FileSink.h"

#define QUILL_DISABLE_NON_PREFIXED_MACROS

#include "quill/LogMacros.h"
#include "quill/Logger.h"

namespace FastLittleMarket {

static constexpr char* QUILL_LOG_FILE = "exchange.log";

// TODO: Create logger for each event type (ADD, CANCEL, MATCH)

#define LOG_INFO(fmt, ...) QUILL_LOG_INFO(infoLogger_, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) \
  QUILL_LOG_WARNING(warningLogger_, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) QUILL_LOG_ERROR(errorLogger_, fmt, ##__VA_ARGS__)

class ExchangeLogger {
 private:
  quill::Logger* infoLogger_;
  quill::Logger* warningLogger_;
  quill::Logger* errorLogger_;

 public:
  ExchangeLogger() {
    quill::Backend::start();
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
        QUILL_LOG_FILE,
        []() {
          quill::FileSinkConfig cfg;
          cfg.set_open_mode('w');
          cfg.set_filename_append_option(
              quill::FilenameAppendOption::StartDateTime);
          return cfg;
        }(),
        quill::FileEventNotifier{});

    infoLogger_ = quill::Frontend::create_or_get_logger(
        "root", std::move(file_sink),
        quill::PatternFormatterOptions{
            "%(time) [%(thread_id)] %(short_source_location:<28) "
            "LOG_%(log_level:<9) %(logger:<12) %(message)",
            "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

    warningLogger_ = quill::Frontend::create_or_get_logger(
        "root", std::move(file_sink),
        quill::PatternFormatterOptions{
            "%(time) [%(thread_id)] %(short_source_location:<28) "
            "LOG_%(log_level:<9) %(logger:<12) %(message)",
            "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

    errorLogger_ = quill::Frontend::create_or_get_logger(
        "root", std::move(file_sink),
        quill::PatternFormatterOptions{
            "%(time) [%(thread_id)] %(short_source_location:<28) "
            "LOG_%(log_level:<9) %(logger:<12) %(message)",
            "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
  }

  ~ExchangeLogger() = default;
};

}  // namespace FastLittleMarket