#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "FixMsg.hpp"
#include "Field.hpp"
#include "FieldList.hpp"

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

  /**
   * @brief add a group to the FieldMap
   *
   * @param tag
   * @return a mutable FieldMap to which fields and groups can be added
   */
  std::optional<FieldMap&> addGroup(MsgType tag);

  void set(MsgType tag, const Field& field);

  const Field& get(MsgType tag) const;

  std::optional<const FieldMap&> getGroup(MsgType tag, size_t index) const;

  std::vector<MsgType> getTags() const { return map.tags(); }

  std::optional<int> getInt(MsgType tag) const {
    auto field = get(tag);
    if (field.isEmpty()) return std::nullopt;
    int value = parseInt(msgBytes + field.offset_,
                         msgBytes + field.offset_ + field.length_);
    return value;
  }

  std::optional<char> getChar(MsgType tag) const {
    auto field = get(tag);
    if (field.isEmpty()) return std::nullopt;

    return *(msgBytes + field.offset_);
  }

  std::optional<long> getLong(MsgType tag) const {
    auto field = get(tag);
    if (field.isEmpty()) return std::nullopt;

    int value = parseLong(msgBytes + field.offset_,
                          msgBytes + field.offset_ + field.length_);
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

  inline std::string_view getString(MsgType tag) const {
    auto field = map->get(tag);
    if (field.isEmpty()) return "";
    auto start = msgBytes + field.offset_;
    return std::string_view(start, field.length_);
  }

  std::vector<MsgType> tags() const { return map->getTags(); }

  // TODO: upgrade to expected<...>
  inline std::optional<int> getInt(MsgType groupTag, size_t index,
                                   MsgType tag) const {
    auto group_opt = map->getGroup(groupTag, index);
    if (!group_opt.has_value()) return std::nullopt;

    auto& grp = group_opt.value();
    auto field = grp.get(tag);
    if (field.isEmpty()) return std::nullopt;

    int value = parseInt(msgBytes + field.offset_,
                         msgBytes + field.offset_ + field.length_);
    return value;
  }

  // TODO: upgrade to expected<...>
  inline std::optional<std::string_view> getString(MsgType groupTag,
                                                   size_t index,
                                                   MsgType tag) const {
    auto group_opt = map->getGroup(groupTag, index);
    if (!group_opt.has_value()) return std::nullopt;

    auto& grp = group_opt.value();
    auto field = grp.get(tag);
    if (field.isEmpty()) return std::nullopt;

    auto start = msgBytes + field.offset_;
    return std::string_view(start, field.length_);
  }

  // TODO: upgrade to expected<...>
  inline const std::optional<FieldAccessor> getGroup(MsgType groupTag,
                                                     size_t index) {
    auto group_opt = map->getGroup(groupTag, index);
    if (!group_opt.has_value()) return std::nullopt;

    auto& grp = group_opt.value();
    return FieldAccessor(&grp);
  }

  // TODO: upgrade to expected<...>
  std::optional<std::vector<MsgType>> tags(MsgType groupTag, int index) const {
    auto group_opt = map->getGroup(groupTag, index);
    if (!group_opt.has_value()) return std::nullopt;

    auto& grp = group_opt.value();
    return grp.getTags();
  }
};

}  // namespace fix

}  // namespace fast_little_market