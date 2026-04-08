#include <array>
#include <string>

namespace FastLittleMarket {

enum class EventType { Add, Cancel, Match };
std::array<std::string, 3> EventTypeNames = {"ADD", "CANCEL", "MATCH"};

}  // namespace FastLittleMarket
