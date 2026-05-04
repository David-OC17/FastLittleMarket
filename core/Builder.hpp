#pragma once

#include <sys/time.h>

#include <charconv>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string_view>

#include "Alloc.hpp"
#include "FieldTag.hpp"

namespace fast_little_market {
namespace fix {

static constexpr std::string_view k_begin_string = "8=FIX.5.0\x01";

class Builder {
 public:
  explicit Builder(size_t max_message_size = 8192)
      : max_message_size_(max_message_size),
        buf_(max_message_size * 2),
        message_(static_cast<char*>(buf_.allocate(max_message_size))),
        cp_(message_) {}

  // Non-copyable - owns a buffer.
  Builder(const Builder&) = delete;
  Builder& operator=(const Builder&) = delete;

  Builder& addField(FieldTag tag, std::string_view value) {
    writeTag(tag);
    write(value);
    writeSOH();
    accountBody();
    return *this;
  }

  Builder& addField(FieldTag tag, int64_t value) {
    writeTag(tag);
    writeInt(value);
    writeSOH();
    accountBody();
    return *this;
  }

  Builder& addField(FieldTag tag, char value) {
    writeTag(tag);
    write(value);
    writeSOH();
    accountBody();
    return *this;
  }

  Builder& addTime(FieldTag tag, const timeval& time) {
    writeTag(tag);
    writeTime(time);
    writeSOH();
    accountBody();
    return *this;
  }

  Builder& addTimeNow(FieldTag tag) {
    timeval tv{};
    gettimeofday(&tv, nullptr);
    return addTime(tag, tv);
  }

  // Appends all fields written so far in src into this builder, then resets
  // src. src must not contain standard header fields.
  Builder& append(Builder& src) {
    const size_t len = src.cp_ - src.message_;
    checkCapacity(len);
    std::memcpy(cp_, src.message_, len);
    cp_ += len;
    if (body_start_) body_length_ += len;
    src.reset();
    return *this;
  }

  // Finalizes the message and returns a view of it.
  // The view is valid until the next call to reset() or build().
  std::string_view build() {
    backfillBodyLength();
    writeChecksum();
    return std::string_view(message_, cp_);
  }

  void reset() noexcept {
    buf_.reset();
    message_ = static_cast<char*>(buf_.allocate(max_message_size_));
    cp_ = message_;
    body_len_dst_ = nullptr;
    body_length_ = 0;

    // k_begin_string not counted in body_length_
    write(k_begin_string);

    // Reserve space for body_length_ field, backfilled during build()
    write(std::string_view("9="));
    body_len_dst_ = cp_;
    std::memset(cp_, '0', BODY_LEN_WIDTH);
    cp_ += BODY_LEN_WIDTH;
    writeSOH();

    body_start_ = cp_;
  }

 private:
  static constexpr size_t BODY_LEN_WIDTH = 6;
  static constexpr size_t CHECKSUM_WIDTH = 7;  // "10=NNN\x01"

  const size_t max_message_size_;
  Buffer buf_;
  char* message_;
  char* cp_;
  char* body_start_ = nullptr;
  char* body_len_dst_ = nullptr;
  size_t body_length_ = 0;

  // TODO: replace throw
  void checkCapacity(size_t needed) const {
    if (static_cast<size_t>(cp_ - message_) + needed + CHECKSUM_WIDTH >
        max_message_size_) {
      throw std::length_error("Builder: message exceeds max_message_size");
    }
  }

  void write(std::string_view sv) {
    checkCapacity(sv.size());
    std::memcpy(cp_, sv.data(), sv.size());
    cp_ += sv.size();
  }

  void write(char c) {
    checkCapacity(1);
    *cp_++ = c;
  }

  void writeInt(int64_t val) {
    // to_chars writes into a local buffer then we copy — no locale, no alloc.
    char tmp[20];
    auto [ptr, ec] = std::to_chars(tmp, tmp + sizeof(tmp), val);
    write(std::string_view(tmp, ptr - tmp));
  }

  void writeTag(FieldTag tag) {
    write(ftof(tag));
    write('=');
  }

  void writeSOH() { write(SOH); }

  void accountBody() {
    if (body_start_) body_length_ = static_cast<size_t>(cp_ - body_start_);
  }

  void writeTime(const timeval& tv) {
    char buf[21];
    struct tm gmt {};
    const time_t secs = tv.tv_sec;
    gmtime_r(&secs, &gmt);

    auto w2 = [](char* p, int v) {
      p[0] = '0' + v / 10;
      p[1] = '0' + v % 10;
    };
    auto w4 = [](char* p, int v) {
      p[0] = '0' + v / 1000;
      p[1] = '0' + (v / 100) % 10;
      p[2] = '0' + (v / 10) % 10;
      p[3] = '0' + v % 10;
    };
    auto w3 = [](char* p, int v) {
      p[0] = '0' + v / 100;
      p[1] = '0' + (v / 10) % 10;
      p[2] = '0' + v % 10;
    };

    w4(buf + 0, gmt.tm_year + 1900);
    w2(buf + 4, gmt.tm_mon + 1);
    w2(buf + 6, gmt.tm_mday);
    buf[8] = '-';
    w2(buf + 9, gmt.tm_hour);
    buf[11] = ':';
    w2(buf + 12, gmt.tm_min);
    buf[14] = ':';
    w2(buf + 15, gmt.tm_sec);
    buf[17] = '.';
    w3(buf + 18, static_cast<int>(tv.tv_usec / 1000));
    write(std::string_view(buf, 21));
  }

  void backfillBodyLength() {
    if (!body_len_dst_) return;
    // Back-fill into the reserved space, right-justified with leading zeros.
    char tmp[BODY_LEN_WIDTH];
    auto [ptr, ec] = std::to_chars(tmp, tmp + BODY_LEN_WIDTH, body_length_);
    const size_t len = ptr - tmp;
    std::memset(body_len_dst_, '0', BODY_LEN_WIDTH - len);
    std::memcpy(body_len_dst_ + BODY_LEN_WIDTH - len, tmp, len);
  }

  void writeChecksum() {
    unsigned int sum = 0;
    for (const char* p = message_; p != cp_; ++p)
      sum += static_cast<unsigned char>(*p);
    sum %= 256;

    char tmp[CHECKSUM_WIDTH] = {'1', '0', '=', '0', '0', '0', '\x01'};
    tmp[6] = '0' + sum % 10;
    sum /= 10;
    tmp[5] = '0' + sum % 10;
    sum /= 10;
    tmp[4] = '0' + sum % 10;
    checkCapacity(CHECKSUM_WIDTH);
    std::memcpy(cp_, tmp, CHECKSUM_WIDTH);
    cp_ += CHECKSUM_WIDTH;
  }
};

}  // namespace fix
}  // namespace fast_little_market