#pragma once

#include <cstdint>
#include <expected>
// #include <map> // TODO implement FieldList with std::map
#include <optional>
#include <span>
#include <vector>
#include <cassert>

#include "Alloc.hpp"
#include "Field.hpp"
#include "FieldTag.hpp"

namespace fast_little_market {

namespace fix {

constexpr size_t FIX_MSG_MAX_FIELD_COUNT = 64;

class FieldList {
 private:
  std::vector<Field, Allocator<Field>> list_;

 public:
  FieldList(Buffer& buffer) : list_(Allocator<Field>(buffer)) {
    list_.reserve(FIX_MSG_MAX_FIELD_COUNT);
  }

  ~FieldList() = default;

  Field& put(const Field& fg) {
    for (auto itr = list_.begin(); itr != list_.end(); itr++) {
      if (itr->tag_ == fg.tag_) {
        FieldMap* savedGroups = itr->groups_;
        FieldMap* savedTail = itr->tail_;
        *itr = fg;
        if (savedGroups && !fg.isGroup()) {
          itr->groups_ = savedGroups;
          itr->tail_ = savedTail;
        }
        return *itr;
      }
    }

    if (list_.size() >= FIX_MSG_MAX_FIELD_COUNT) {
      assert(false && "FieldList capacity exceeded");
      return list_.back();  // unreachable, satisfies return type
    }
    list_.push_back(fg);
    return list_.back();
  }

  Field* get(const FieldTag tag) {
    for (auto itr = list_.begin(); itr != list_.end(); itr++) {
      if (itr->tag_ == tag) {
        return &(*itr);
      }
    }
    return nullptr;
  }

  bool contains(const FieldTag tag) const {
    for (auto itr = list_.begin(); itr != list_.end(); itr++) {
      if (itr->tag_ == tag) {
        return true;
      }
    }
    return false;
  }

  Field* find(FieldTag tag) {
    for (auto& f : list_)
      if (f.tag_ == tag) return &f;
    return nullptr;
  }

  const Field* find(FieldTag tag) const {
    for (const auto& f : list_)
      if (f.tag_ == tag) return &f;
    return nullptr;
  }

  std::span<const Field> fields() const { return std::span<const Field>(list_); }
};

}  // namespace fix

}  // namespace fast_little_market
