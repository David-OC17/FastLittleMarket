#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Alloc.hpp"
#include "Field.hpp"
#include "FieldList.hpp"
#include "FieldTag.hpp"

namespace fast_little_market {

namespace fix {

namespace {

static inline int parseInt(const char* start, const char* endExclusive) {
  int value = 0;
  bool negate = false;
  if (*start == '-') {
    negate = true;
    start++;
  }
  while (start != endExclusive) {
    value *= 10;
    value += *start - '0';
    start++;
  }
  return negate ? value * -1 : value;
}

static inline long parseLong(const char* start, const char* endExclusive) {
  long value = 0;
  bool negate = false;
  if (*start == '-') {
    negate = true;
    start++;
  }
  while (start != endExclusive) {
    value *= 10;
    value += *start - '0';
    start++;
  }
  return negate ? value * -1 : value;
}

}  // namespace

class FieldMap {
  friend struct Field;
  friend class FieldAccessor;

  Buffer& buffer_;
  const char* msgBytes_;
  FieldMap* next_ = nullptr;  // linked list for group
  FieldList map_;

 public:
  FieldMap(Buffer& buffer, const char* msgBytes)
      : buffer_(buffer), msgBytes_(msgBytes), map_() {}

  FieldMap* addGroup(FieldTag tag);

  void set(const Field& field);

  const Field* get(FieldTag tag) const;

  FieldMap* getGroup(FieldTag tag, size_t index) const;

  std::span<const Field> getFields() const { return map_.fields(); }

  std::optional<int> getInt(FieldTag tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;
    int value = parseInt(msgBytes_ + field->offset_,
                         msgBytes_ + field->offset_ + field->length_);
    return value;
  }

  std::optional<char> getChar(FieldTag tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;

    return *(msgBytes_ + field->offset_);
  }

  std::optional<long> getLong(FieldTag tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;

    long value = parseLong(msgBytes_ + field->offset_,
                           msgBytes_ + field->offset_ + field->length_);
    return value;
  }

  std::optional<int> getInt(const Field& field) const {
    if (field.isEmpty()) return std::nullopt;

    int value = parseInt(msgBytes_ + field.offset_,
                         msgBytes_ + field.offset_ + field.length_);
    return value;
  }
};

// all FieldAccessor methods are only valid for the lifetime of the source
// message
class FieldAccessor {
  friend class FixMessage;

 private:
  const FieldMap* map_;
  const char* msgBytes_;

  FieldAccessor(const FieldMap* map) : map_(map), msgBytes_(map->msgBytes_) {}

  FieldAccessor() : map_(nullptr), msgBytes_(nullptr) {}

  void reset(const FieldMap* map) {
    map_ = map;
    msgBytes_ = map->msgBytes_;
  }

 public:
  inline std::optional<int> getInt(FieldTag tag) const {
    return map_->getInt(tag);
  }

  inline std::optional<char> getChar(FieldTag tag) const {
    return map_->getChar(tag);
  }

  inline std::optional<long> getLong(FieldTag tag) const {
    return map_->getLong(tag);
  }

  inline std::optional<std::string_view> getString(FieldTag tag) const {
    auto field = map_->get(tag);
    if (field == nullptr) return std::nullopt;

    auto start = msgBytes_ + field->offset_;
    return std::string_view(start, field->length_);
  }

  std::span<const Field> tags() const { return map_->getFields(); }

  inline std::optional<int> getInt(FieldTag groupTag, size_t index,
                                   FieldTag tag) const {
    FieldMap* grp = map_->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    const Field* field = grp->get(tag);
    if (field == nullptr) return std::nullopt;

    return parseInt(grp->msgBytes_ + field->offset_,
                    grp->msgBytes_ + field->offset_ + field->length_);
  }

  inline std::optional<std::string_view> getString(FieldTag groupTag,
                                                   size_t index,
                                                   FieldTag tag) const {
    FieldMap* grp = map_->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    const Field* field = grp->get(tag);
    if (field == nullptr) return std::nullopt;

    return std::string_view(grp->msgBytes_ + field->offset_, field->length_);
  }

  inline std::optional<FieldAccessor> getGroup(FieldTag groupTag,
                                               size_t index) {
    FieldMap* grp = map_->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    return FieldAccessor(grp);
  }

  std::optional<std::span<const Field>> tags(FieldTag groupTag,
                                             size_t index) const {
    FieldMap* grp = map_->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    return grp->getFields();
  }
};

}  // namespace fix

}  // namespace fast_little_market