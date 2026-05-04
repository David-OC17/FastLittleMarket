#pragma once

#include <cstdint>
#include <cstring>
#include <istream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Alloc.hpp"
#include "FieldMap.hpp"
#include "FieldTag.hpp"
#include "MsgType.hpp"

namespace fast_little_market {
namespace fix {

struct GroupDef {
  FieldTag group_count_tag_;
  FieldTag group_end_tag_;
};

class GroupDefs {
  std::unordered_map<std::string, std::vector<GroupDef>> defs_;

 public:
  void add(std::string_view msg_type, GroupDef def) {
    defs_[std::string(msg_type)].push_back(def);
  }

  void add(std::string_view msg_type, std::vector<GroupDef> defs) {
    defs_[std::string(msg_type)] = std::move(defs);
  }

  const std::vector<GroupDef>* defs(std::string_view msg_type) const {
    auto itr = defs_.find(std::string(msg_type));
    return itr == defs_.end() ? nullptr : &itr->second;
  }
};

class FixMessage final : public FieldAccessor {
 private:
  const int max_message_size_;
  Buffer buf_;
  char* msg_bytes_;
  FieldMap* map_;

  void reset() {
    buf_.reset();
    void* map_mem = buf_.allocate(sizeof(FieldMap));
    msg_bytes_ = static_cast<char*>(buf_.allocate(max_message_size_));
    std::memset(msg_bytes_, 0, max_message_size_);
    map_ = new (map_mem) FieldMap(buf_, msg_bytes_);
    FieldAccessor::reset(map_);
  }

  static_assert(std::is_trivially_destructible_v<FieldMap>,
                "FieldMap must be trivially destructible for arena reuse");

 public:
  FixMessage() : FixMessage(2048) {}

  explicit FixMessage(int max_message_size)
      : max_message_size_(max_message_size), buf_(max_message_size * 4) {
    void* map_mem = buf_.allocate(sizeof(FieldMap));
    msg_bytes_ = static_cast<char*>(buf_.allocate(max_message_size));
    std::memset(msg_bytes_, 0, max_message_size);
    map_ = new (map_mem) FieldMap(buf_, msg_bytes_);
    FieldAccessor::reset(map_);
  }
  static void parse(std::istream& in, FixMessage& msg, const GroupDefs& defs);

  static void parse(std::string_view in, FixMessage& msg,
                    const GroupDefs& defs);
};

}  // namespace fix
}  // namespace fast_little_market