#include "FieldMap.hpp"

namespace fast_little_market {

namespace fix {

FieldMap* FieldMap::addGroup(MsgType tag) {
  void* mem = nullptr;
  try {
    mem = buffer.allocate(sizeof(FieldMap));
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
  if (!mem) return nullptr;

  FieldMap* fm = new (mem) FieldMap(buffer, msgBytes);
  Field* itr = map.find(tag);
  if (itr == nullptr) {
    Field f(tag, 0, 0, fm);
    map.put(f);
    return fm;
  }
  if (!itr->isGroup()) {
    itr->groups_ = fm;
    return fm;
  }
  FieldMap* tail = itr->groups_;
  while (tail->next != nullptr) tail = tail->next;
  tail->next = fm;
  return fm;
}

void FieldMap::set(const Field& field) { map.put(field); }

// Returns nullptr if not found — callers must null-check
Field* FieldMap::get(MsgType tag) const { return map.find(tag); }

// Returns nullptr if tag not found, not a group, or index out of range
FieldMap* FieldMap::getGroup(MsgType tag, size_t index) const {
  Field* itr = map.find(tag);
  if (itr == nullptr || !itr->isGroup()) return nullptr;
  FieldMap* group = itr->group(index);
  return group;
}

FieldMap* Field::group(size_t n) const {
  FieldMap* ptr = groups_;
  while (n-- > 0) {
    if (ptr == nullptr) return nullptr;  // index out of range
    ptr = ptr->next;
  }
  return ptr;
}

size_t Field::groupCount() const {
  FieldMap* ptr = groups_;
  size_t count = 0;
  while (ptr != nullptr) {
    count++;
    ptr = ptr->next;
  }
  return count;
}

}  // namespace fix

}  // namespace fast_little_market