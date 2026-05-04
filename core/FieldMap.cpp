#include "FieldMap.hpp"

#include <memory>

namespace fast_little_market {

namespace fix {

FieldMap* FieldMap::addGroup(FieldTag tag) {
  // Allocate raw memory for the child FieldMap from the arena
  void* mem = nullptr;
  try {
    mem = buffer_.allocate(sizeof(FieldMap));
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
  if (!mem) return nullptr;

  // Construct the child FieldMap in-place; may throw if arena can't fit
  // reserve()
  FieldMap* fm;
  try {
    fm = new (mem) FieldMap(buffer_, msg_bytes_);
  } catch (const std::bad_alloc&) {
    return nullptr;
  }

  Field* itr = map_.find(tag);
  if (itr == nullptr) {
    // First group for this tag: create a sentinel Field pointing to the first
    // child
    Field f(tag, 0, 0, fm);
    Field& stored = map_.put(f);
    (void)stored;
    return fm;
  }

  if (!itr->isGroup()) {
    // Field exists but wasn't a group yet: attach first child
    itr->groups_ = fm;
    itr->tail_ = fm;
    return fm;
  }

  // Append to the existing group linked list
  itr->tail_->next_ = fm;
  itr->tail_ = fm;
  return fm;
}

void FieldMap::set(const Field& field) { map_.put(field); }

// TODO: returns nullptr if not found — callers must null-check --> report via
// throw?
const Field* FieldMap::get(FieldTag tag) const { return map_.find(tag); }

// TODO: returns nullptr if tag not found, not a group, or index out of range
// --> report via throw?
FieldMap* FieldMap::getGroup(FieldTag tag, size_t index) const {
  const Field* itr = map_.find(tag);
  if (itr == nullptr || !itr->isGroup()) return nullptr;
  FieldMap* group = itr->group(index);
  return group;
}

// TODO: change index-out-of-range return behavior; clarify result
FieldMap* Field::group(size_t n) const {
  FieldMap* ptr = groups_;
  while (n-- > 0) {
    if (ptr == nullptr) return nullptr;  // index out of range
    ptr = ptr->next_;
  }
  return ptr;
}

size_t Field::groupCount() const {
  FieldMap* ptr = groups_;
  size_t count = 0;
  while (ptr != nullptr) {
    count++;
    ptr = ptr->next_;
  }
  return count;
}

}  // namespace fix

}  // namespace fast_little_market