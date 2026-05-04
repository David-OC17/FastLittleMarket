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

static inline int parseInt(const char* start, const char* end_exclusive) {
  int value = 0;
  bool negate = false;
  if (*start == '-') {
    negate = true;
    start++;
  }
  while (start != end_exclusive) {
    value *= 10;
    value += *start - '0';
    start++;
  }
  return negate ? value * -1 : value;
}

static inline long parseLong(const char* start, const char* end_exclusive) {
  long value = 0;
  bool negate = false;
  if (*start == '-') {
    negate = true;
    start++;
  }
  while (start != end_exclusive) {
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
  const char* msg_bytes_;
  FieldMap* next_ = nullptr;  // linked list for group
  FieldList map_;

 public:
  FieldMap(Buffer& buffer, const char* msg_bytes)
      : buffer_(buffer), msg_bytes_(msg_bytes), map_() {}

  FieldMap* addGroup(FieldTag tag);

  void set(const Field& field);

  const Field* get(FieldTag tag) const;

  FieldMap* getGroup(FieldTag tag, size_t index) const;

  std::span<const Field> getFields() const { return map_.fields(); }

  std::optional<int> getInt(FieldTag tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;
    int value = parseInt(msg_bytes_ + field->offset_,
                         msg_bytes_ + field->offset_ + field->length_);
    return value;
  }

  std::optional<char> getChar(FieldTag tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;

    return *(msg_bytes_ + field->offset_);
  }

  std::optional<long> getLong(FieldTag tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;

    long value = parseLong(msg_bytes_ + field->offset_,
                           msg_bytes_ + field->offset_ + field->length_);
    return value;
  }

  std::optional<int> getInt(const Field& field) const {
    if (field.isEmpty()) return std::nullopt;

    int value = parseInt(msg_bytes_ + field.offset_,
                         msg_bytes_ + field.offset_ + field.length_);
    return value;
  }
};

// all FieldAccessor methods are only valid for the lifetime of the source
// message
class FieldAccessor {
  friend class FixMessage;

 private:
  const FieldMap* map_;
  const char* msg_bytes_;

  FieldAccessor(const FieldMap* map) : map_(map), msg_bytes_(map->msg_bytes_) {}

  FieldAccessor() : map_(nullptr), msg_bytes_(nullptr) {}

  void reset(const FieldMap* map) {
    map_ = map;
    msg_bytes_ = map->msg_bytes_;
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

    auto start = msg_bytes_ + field->offset_;
    return std::string_view(start, field->length_);
  }

  std::span<const Field> tags() const { return map_->getFields(); }

  inline std::optional<int> getInt(FieldTag group_tag, size_t index,
                                   FieldTag tag) const {
    FieldMap* grp = map_->getGroup(group_tag, index);
    if (grp == nullptr) return std::nullopt;

    const Field* field = grp->get(tag);
    if (field == nullptr) return std::nullopt;

    return parseInt(grp->msg_bytes_ + field->offset_,
                    grp->msg_bytes_ + field->offset_ + field->length_);
  }

  inline std::optional<std::string_view> getString(FieldTag group_tag,
                                                   size_t index,
                                                   FieldTag tag) const {
    FieldMap* grp = map_->getGroup(group_tag, index);
    if (grp == nullptr) return std::nullopt;

    const Field* field = grp->get(tag);
    if (field == nullptr) return std::nullopt;

    return std::string_view(grp->msg_bytes_ + field->offset_, field->length_);
  }

  inline std::optional<FieldAccessor> getGroup(FieldTag group_tag,
                                               size_t index) {
    FieldMap* grp = map_->getGroup(group_tag, index);
    if (grp == nullptr) return std::nullopt;

    return FieldAccessor(grp);
  }

  std::optional<std::span<const Field>> tags(FieldTag group_tag,
                                             size_t index) const {
    FieldMap* grp = map_->getGroup(group_tag, index);
    if (grp == nullptr) return std::nullopt;

    return grp->getFields();
  }
};

}  // namespace fix

}  // namespace fast_little_market