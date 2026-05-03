#pragma once

#include <cstdint>

#include "FieldTag.hpp"

namespace fast_little_market {

namespace fix {

class FieldMap;

class Field {
 public:
  FieldMap* groups_;
  FieldMap* tail_ = nullptr;
  uint16_t offset_;
  uint16_t length_;
  FieldTag tag_;  // 16-bit

  constexpr Field()
      : groups_(nullptr),
        tail_(nullptr),
        offset_(0),
        length_(0),
        tag_(FieldTag::INVALID) {}

  Field(FieldTag tag, uint16_t offset, uint16_t length, FieldMap* groups)
      : groups_(groups),
        tail_(groups),
        offset_(offset),
        length_(length),
        tag_(tag) {}

  Field(FieldTag tag, uint16_t offset, uint16_t length)
      : groups_(nullptr),
        tail_(nullptr),
        offset_(offset),
        length_(length),
        tag_(tag) {}

  inline bool isEmpty() const { return tag_ == FieldTag::INVALID; }

  inline bool isGroup() const { return groups_ != nullptr; }

  Field(const Field& f) = default;

  Field& operator=(const Field& f) = default;

  FieldMap* group(size_t n) const;

  size_t groupCount() const;
};

}  // namespace fix

}  // namespace fast_little_market