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
  FieldTag groupCountTag;
  FieldTag groupEndTag;
};

class GroupDefs {
  std::unordered_map<std::string, std::vector<GroupDef>> defs_;

 public:
  void add(std::string_view msgType, GroupDef def) {
    defs_[std::string(msgType)].push_back(def);
  }

  void add(std::string_view msgType, std::vector<GroupDef> defs) {
    defs_[std::string(msgType)] = std::move(defs);
  }

  const std::vector<GroupDef>* defs(std::string_view msgType) const {
    auto itr = defs_.find(std::string(msgType));
    return itr == defs_.end() ? nullptr : &itr->second;
  }
};

class FixMessage final : public FieldAccessor {
 private:
  const int maxMessageSize_;
  Buffer buf_;
  char* msgBytes_;
  FieldMap* map_;

  void reset() {
    buf_.reset();
    void* mapMem = buf_.allocate(sizeof(FieldMap));
    msgBytes_ = static_cast<char*>(buf_.allocate(maxMessageSize_));
    std::memset(msgBytes_, 0, maxMessageSize_);
    map_ = new (mapMem) FieldMap(buf_, msgBytes_);
    FieldAccessor::reset(map_);
  }

  static_assert(std::is_trivially_destructible_v<FieldMap>,
                "FieldMap must be trivially destructible for arena reuse");

 public:
  FixMessage() : FixMessage(2048) {}

  explicit FixMessage(int maxMessageSize)
      : maxMessageSize_(maxMessageSize), buf_(maxMessageSize * 4) {
    void* mapMem = buf_.allocate(sizeof(FieldMap));
    msgBytes_ = static_cast<char*>(buf_.allocate(maxMessageSize));
    std::memset(msgBytes_, 0, maxMessageSize);
    map_ = new (mapMem) FieldMap(buf_, msgBytes_);
    FieldAccessor::reset(map_);
  }
  static void parse(std::istream& in, FixMessage& msg, const GroupDefs& defs);

  static void parse(std::string_view in, FixMessage& msg,
                    const GroupDefs& defs);
};

}  // namespace fix
}  // namespace fast_little_market