#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>

#include "Field.hpp"
#include "FieldTag.hpp"

namespace fast_little_market {
namespace fix {

constexpr size_t FIX_MSG_MAX_FIELD_COUNT = 64;

class FieldList {
 private:
  std::array<Field, FIX_MSG_MAX_FIELD_COUNT> list_;
  size_t size_ = 0;

 public:
  FieldList() = default;
  ~FieldList() = default;

  Field& put(const Field& fg) {
    for (size_t i = 0; i < size_; i++) {
      if (list_[i].tag_ == fg.tag_) {
        FieldMap* saved_groups = list_[i].groups_;
        FieldMap* saved_tail = list_[i].tail_;
        list_[i] = fg;
        if (saved_groups && !fg.isGroup()) {
          list_[i].groups_ = saved_groups;
          list_[i].tail_ = saved_tail;
        }
        return list_[i];
      }
    }
    assert(size_ < FIX_MSG_MAX_FIELD_COUNT && "FieldList capacity exceeded");
    list_[size_] = fg;
    return list_[size_++];
  }

  Field* find(FieldTag tag) {
    for (size_t i = 0; i < size_; i++)
      if (list_[i].tag_ == tag) return &list_[i];
    return nullptr;
  }

  const Field* find(FieldTag tag) const {
    for (size_t i = 0; i < size_; i++)
      if (list_[i].tag_ == tag) return &list_[i];
    return nullptr;
  }

  bool contains(FieldTag tag) const { return find(tag) != nullptr; }

  std::span<const Field> fields() const {
    return std::span<const Field>(list_.data(), size_);
  }

  size_t size() const { return size_; }
};

}  // namespace fix
}  // namespace fast_little_market