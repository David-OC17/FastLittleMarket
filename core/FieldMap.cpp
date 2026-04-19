#include "FieldMap.hpp"

namespace fast_little_market {

namespace fix {

// TODO: upgrade to expected<FieldMap&, ERROR> and pass err reason
std::optional<FieldMap&> FieldMap::addGroup(MsgType tag) {
  auto itr = map.find(tag);
  if (itr == nullptr) return std::nullopt;

  if (!itr->isGroup()) {
    auto alloc_res = buffer.allocate(sizeof(FieldMap));
    if (!alloc_res) return std::nullopt;  // AllocError::out_of_memory;

    FieldMap* obj = (FieldMap*)alloc_res.value();
    auto fm = new (obj) FieldMap(buffer, msgBytes);
    itr->groups_ = fm;
    return *const_cast<FieldMap*>(itr->groups_);
  }

  FieldMap* tail = itr->groups_;
  while (tail->next != nullptr) tail = tail->next;

  auto alloc_res = buffer.allocate(sizeof(FieldMap));
  if (!alloc_res) return std::nullopt;  // AllocError::out_of_memory;

  FieldMap* obj = (FieldMap*)alloc_res.value();
  tail->next = new (obj) FieldMap(buffer, msgBytes);
  return *tail->next;
}

void FieldMap::set(MsgType tag, const Field& field) { map.put(field); }

const Field& FieldMap::get(MsgType tag) const {
  auto itr = map.find(tag);
  if (itr != nullptr) {
    return *itr;
  } else {
    return Field();
  }
}

std::optional<const FieldMap&> FieldMap::getGroup(MsgType tag,
                                                  size_t index) const {
  auto itr = map.find(tag);
  if (itr == nullptr || !itr->isGroup()) {
    return std::nullopt;
  }

  return *itr->group(index);
}

FieldMap* Field::group(size_t n) const {
  auto ptr = groups_;
  while (n-- > 0) {
    ptr = ptr->next;
  }
  return ptr;
}

size_t Field::groupCount() const {
  auto ptr = groups_;
  int count = 0;
  while (ptr != nullptr) {
    count++;
    ptr = ptr->next;
  }
  return count;
}

}  // namespace fix

}  // namespace fast_little_market