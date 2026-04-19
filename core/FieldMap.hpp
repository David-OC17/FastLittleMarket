#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "Field.hpp"
#include "FieldList.hpp"
#include "FixMsg.hpp"

namespace fast_little_market {

namespace fix {

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

class FieldMap {
  friend struct Field;
  friend class FieldAccessor;

  Buffer& buffer;
  const char* msgBytes;
  FieldMap* next = nullptr;  // linked list for group
  FieldList map;

 public:
  FieldMap(Buffer& buffer, const char* msgBytes)
      : buffer(buffer), msgBytes(msgBytes), map(buffer) {}

  FieldMap* addGroup(MsgType tag);

  void set(const Field& field);

  Field* get(MsgType tag) const;

  FieldMap* getGroup(MsgType tag, size_t index) const;

  std::vector<MsgType> getTags() const { return map.tags(); }

  std::optional<int> getInt(MsgType tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;
    int value = parseInt(msgBytes + field->offset_,
                         msgBytes + field->offset_ + field->length_);
    return value;
  }

  std::optional<char> getChar(MsgType tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;

    return *(msgBytes + field->offset_);
  }

  std::optional<long> getLong(MsgType tag) const {
    auto field = get(tag);
    if (field == nullptr) return std::nullopt;

    long value = parseLong(msgBytes + field->offset_,
                           msgBytes + field->offset_ + field->length_);
    return value;
  }

  std::optional<int> getInt(const Field& field) const {
    if (field.isEmpty()) return std::nullopt;

    int value = parseInt(msgBytes + field.offset_,
                         msgBytes + field.offset_ + field.length_);
    return value;
  }
};

// all FieldAccessor methods are only valid for the lifetime of the source
// message
class FieldAccessor {
  friend class FixMessage;

 private:
  const FieldMap* map;
  const char* msgBytes;

  FieldAccessor(const FieldMap* map) : map(map), msgBytes(map->msgBytes) {}

  FieldAccessor() = delete;

  void reset(const FieldMap* map) {
    this->map = map;
    this->msgBytes = map->msgBytes;
  }

 public:
  inline std::optional<int> getInt(MsgType tag) const {
    return map->getInt(tag);
  }

  inline std::optional<char> getChar(MsgType tag) const {
    return map->getChar(tag);
  }

  inline std::optional<long> getLong(MsgType tag) const {
    return map->getLong(tag);
  }

  inline std::optional<std::string_view> getString(MsgType tag) const {
    auto field = map->get(tag);
    if (field == nullptr) return std::nullopt;

    auto start = msgBytes + field->offset_;
    return std::string_view(start, field->length_);
  }

  std::vector<MsgType> tags() const { return map->getTags(); }

  inline std::optional<int> getInt(MsgType groupTag, size_t index,
                                   MsgType tag) const {
    FieldMap* grp = map->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    Field* field = grp->get(tag);
    if (field == nullptr) return std::nullopt;

    return parseInt(msgBytes + field->offset_,
                    msgBytes + field->offset_ + field->length_);
  }

  inline std::optional<std::string_view> getString(MsgType groupTag,
                                                   size_t index,
                                                   MsgType tag) const {
    FieldMap* grp = map->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    Field* field = grp->get(tag);
    if (field == nullptr) return std::nullopt;

    return std::string_view(msgBytes + field->offset_, field->length_);
  }

  inline std::optional<FieldAccessor> getGroup(MsgType groupTag, size_t index) {
    FieldMap* grp = map->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    return FieldAccessor(grp);
  }

  std::optional<std::vector<MsgType>> tags(MsgType groupTag, int index) const {
    FieldMap* grp = map->getGroup(groupTag, index);
    if (grp == nullptr) return std::nullopt;

    return grp->getTags();
  }
};

}  // namespace fix

}  // namespace fast_little_market