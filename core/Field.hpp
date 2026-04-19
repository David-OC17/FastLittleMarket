#pragma once

#include <cstdint>

#include "FixMsg.hpp"

namespace fast_little_market {

namespace fix {

class FieldMap;

class Field {
 public:
  size_t offset_;               // at least 16-bit
  size_t length_;               // at least 16-bit
  MsgType tag_;                 // 16-bit
  FieldMap* groups_ = nullptr;  // (assume) 8-bit

  inline bool isEmpty() const { return tag_ == MsgType::INVALID; }

  inline bool isGroup() const { return groups_ != nullptr; }

  constexpr Field()
      : tag_(MsgType::INVALID), offset_(0), length_(0), groups_(nullptr) {}

  Field(MsgType tag, size_t offset, size_t length, FieldMap* groups)
      : tag_(tag), offset_(offset), length_(length), groups_(groups) {}

  Field(MsgType tag, size_t offset, size_t length)
      : tag_(tag), offset_(offset), length_(length), groups_(nullptr) {}

  Field(const Field& f) {
    tag_ = f.tag_;
    offset_ = f.offset_;
    length_ = f.length_;
    groups_ = f.groups_;
  }

  Field& operator=(const Field& f) {
    tag_ = f.tag_;
    offset_ = f.offset_;
    length_ = f.length_;
    groups_ = f.groups_;
    return *this;
  }

  FieldMap* group(size_t n) const;

  size_t groupCount() const;
};

}  // namespace fix

}  // namespace fast_little_market