#include "Parser.hpp"

namespace fast_little_market {
namespace fix {

// void FixMessage::parse(const char* in, FixMessage& msg, const GroupDefs& defs) {
//   // Wrap in a lightweight string_view-based stream to avoid istrstream
//   // (deprecated) while keeping a single implementation path.
//   // We still delegate to the istream overload via a simple wrapper.
//   struct CharStream : std::istream {
//     struct Buf : std::streambuf {
//       Buf(const char* s) {
//         auto p = const_cast<char*>(s);
//         setg(p, p, p + __builtin_strlen(s));
//       }
//     } buf_;
//     explicit CharStream(const char* s) : std::istream(&buf_), buf_(s) {}
//   } stream(in);
//   parse(stream, msg, defs);
// }

void FixMessage::parse(std::istream& in, FixMessage& msg,
                       const GroupDefs& defs) {
  msg.reset();

  char* const msgBytes = msg.msgBytes_;
  char* end = msgBytes;    // one-past the last byte read into the buffer
  char* cp = msgBytes;     // current scan position
  char* start = msgBytes;  // start of the current token

  const std::vector<GroupDef>* msgGroups = nullptr;
  std::vector<FieldMap*> stack;
  FieldMap* map = msg.map_;

  // Lazily extend the buffer by one character.
  auto readOne = [&]() -> bool {
    if (!in.get(*end)) return false;
    ++end;
    return true;
  };

  // Ensure cp < end, reading if necessary.
  auto ensureChar = [&]() -> bool {
    if (cp < end) return true;
    return readOne();
  };

  // Read the mandatory minimum (8=FIX.x.y\x01 9=nn\x01 ... 10=x\x01 = 37
  // bytes).
  if (!in.read(end, 37)) return;
  end += 37;

  while (true) {
    // Scan to '='  →  tag number
    while (true) {
      if (!ensureChar()) return;
      if (*cp == '=') break;
      ++cp;
    }

    const FieldTag tag = [&]() -> FieldTag {
      // Convert the ASCII integer directly to a FieldTag via stoft.
      // stoft expects the tag number as a string_view.
      auto result = stoft(std::string_view(start, cp - start));
      return result.value_or(FieldTag::INVALID);
    }();

    ++cp;
    start = cp;

    // Scan to SOH  →  field value
    while (true) {
      if (!ensureChar()) return;
      if (*cp == '\x01') break;
      ++cp;
    }

    const int offset = static_cast<int>(start - msgBytes);
    const int length = static_cast<int>(cp - start);

    if (tag == FieldTag::CheckSum) {
      // CheckSum is always the last field; nothing should remain buffered.
      return;
    }

    map->set(Field(tag, offset, length));

    if (tag == FieldTag::BodyLength) {
      // Body length excludes the trailing 10=NNN\x01 (7 bytes).
      const int bodyLength = map->getInt(tag).value_or(0);
      const int toRead = (bodyLength + 7) - static_cast<int>(end - cp);
      if (toRead > 0) {
        if (!in.read(end, toRead)) return;
        end += toRead;
      }
    }

    if (tag == FieldTag::MsgType) {
      const auto msgType = std::string_view(msgBytes + offset, length);
      msgGroups = defs.defs(msgType);
    }

    // Skip the SOH delimiter and advance start to next field.
    ++cp;
    start = cp;

    if (msgGroups == nullptr) continue;

    for (const auto& grpDef : *msgGroups) {
      if (grpDef.groupCountTag == tag) {
        stack.push_back(map);
        map = map->addGroup(tag);
        break;
      }

      if (grpDef.groupEndTag == tag) {
        if (stack.empty()) break;

        FieldMap* parent = stack.back();
        const Field* countField = parent->get(grpDef.groupCountTag);
        if (countField == nullptr) break;

        const int expected = parent->getInt(*countField).value_or(0);
        const int received = static_cast<int>(countField->groupCount());

        if (received >= expected) {
          // All groups accounted for — pop back to parent.
          stack.pop_back();
          map = parent;
        } else {
          // More sibling groups expected — open a new one on the parent.
          map = parent->addGroup(grpDef.groupCountTag);
        }
        break;
      }
    }
  }
}

}  // namespace fix
}  // namespace fast_little_market