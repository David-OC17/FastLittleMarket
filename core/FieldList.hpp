#pragma once

#include <cstdint>
#include <expected>
// #include <map> // TODO implement FieldList with std::map
#include <optional>
#include <vector>

#include "Alloc.hpp"
#include "Field.hpp"
#include "FixMsg.hpp"

namespace fast_little_market {

namespace fix {

constexpr size_t FIX_MSG_MAX_FIELD_COUNT = 64;

class FieldList {
 private:
  using VecAllocatorType = Allocator<Field>;

  std::vector<Field, VecAllocatorType> list;

 public:
  FieldList(Buffer& buffer) : list(VecAllocatorType(buffer)) {
    list.reserve(FIX_MSG_MAX_FIELD_COUNT);
  }

  Field& put(const Field& fg) {
    for (auto itr = list.begin(); itr != list.end(); itr++) {
      if (itr->tag_ == fg.tag_) {
        *itr = fg;
        return *itr;
      }
    }
    list.push_back(fg);
    return list.back();
  }

  Field* get(const MsgType tag) {
    for (auto itr = list.begin(); itr != list.end(); itr++) {
      if (itr->tag_ == tag) {
        return &(*itr);
      }
    }
    return nullptr; 
  }

  bool contains(const MsgType tag) const {
    for (auto itr = list.begin(); itr != list.end(); itr++) {
      if (itr->tag_ == tag) {
        return true;
      }
    }
    return false;
  }

  Field* find(const MsgType tag) const {
    for (auto itr = list.begin(); itr != list.end(); itr++) {
      if (itr->tag_ == tag) {
        return const_cast<Field*>(&(*itr));
      }
    }
    return nullptr;
  }

  std::vector<MsgType> tags() const {
    std::vector<MsgType> tags{};
    tags.reserve(list.size());
    for (auto field : list) {
      tags.push_back(field.tag_);
    }
    return tags;
  }
};

}  // namespace fix

}  // namespace fast_little_market
