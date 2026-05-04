#include "Parser.hpp"

namespace fast_little_market {
namespace fix {

// NOTE: avoid in prod, creates extra string copy
void FixMessage::parse(std::string_view in, FixMessage& msg,
                       const GroupDefs& defs) {
  std::string owned{in};
  std::istringstream stream{std::move(owned)};
  parse(stream, msg, defs);
}

void FixMessage::parse(std::istream& in, FixMessage& msg,
                       const GroupDefs& defs) {
  msg.reset();

  char* const msg_bytes = msg.msg_bytes_;
  char* end = msg_bytes;    // one-past the last byte read into the buffer
  char* cp = msg_bytes;     // current scan position
  char* start = msg_bytes;  // start of the current token

  const std::vector<GroupDef>* msg_groups = nullptr;
  std::vector<FieldMap*> stack;
  FieldMap* map = msg.map_;

  // Lazily extend the buffer by one character.
  auto read_one = [&]() -> bool {
    if (!in.get(*end)) return false;
    ++end;
    return true;
  };

  // Ensure cp < end, reading if necessary.
  auto ensure_char = [&]() -> bool {
    if (cp < end) return true;
    return read_one();
  };

  // Read mandatory minimum (8=FIX.x.y\x01 9=nn\x01 ... 10=x\x01 = 37 bytes).
  // if (!in.read(end, 37)) return;
  // end += 37;

  while (true) {
    // Scan to '='  →  tag number
    while (true) {
      if (!ensure_char()) return;
      if (*cp == '=') break;
      ++cp;
    }

    const FieldTag tag = [&]() -> FieldTag {
      auto sv = std::string_view(start, cp - start);
      auto result = stoft(sv);
      return result.value_or(FieldTag::INVALID);
    }();

    ++cp;
    start = cp;

    // Scan to SOH  →  field value
    while (true) {
      if (!ensure_char()) return;
      if (*cp == '\x01') break;
      ++cp;
    }

    const int offset = static_cast<int>(start - msg_bytes);
    const int length = static_cast<int>(cp - start);

    if (tag == FieldTag::CheckSum) {
      // CheckSum is always the last field; nothing should remain buffered.
      return;
    }

    map->set(Field(tag, offset, length));

    if (tag == FieldTag::BodyLength) {
      // Body length excludes the trailing 10=NNN\x01 (7 bytes).
      const int body_length = map->getInt(tag).value_or(0);
      const int to_read = (body_length + 7) - static_cast<int>(end - cp);
      if (to_read > 0) {
        if (!in.read(end, to_read)) return;
        end += to_read;
      }
    }

    if (tag == FieldTag::MsgType) {
      const auto msg_type = std::string_view(msg_bytes + offset, length);
      msg_groups = defs.defs(msg_type);
    }

    // Skip the SOH delimiter and advance start to next field.
    ++cp;
    start = cp;

    if (msg_groups == nullptr) continue;

    for (const auto& grp_def : *msg_groups) {
      if (grp_def.group_count_tag_ == tag) {
        stack.push_back(map);
        map = map->addGroup(tag);
        break;
      }

      if (grp_def.group_end_tag_ == tag) {
        if (stack.empty()) break;

        FieldMap* parent = stack.back();
        const Field* count_field = parent->get(grp_def.group_count_tag_);
        if (count_field == nullptr) break;

        const int expected = parent->getInt(*count_field).value_or(0);
        const int received = static_cast<int>(count_field->groupCount());

        if (received >= expected) {
          // All groups accounted for — pop back to parent.
          stack.pop_back();
          map = parent;
        } else {
          // More sibling groups expected — open a new one on the parent.
          map = parent->addGroup(grp_def.group_count_tag_);
        }
        break;
      }
    }
  }
}

}  // namespace fix
}  // namespace fast_little_market