#pragma once

#include <cstdint>
#include <string_view>

namespace fast_little_market {

constexpr uint64_t fnv1a(const std::string_view str,
                         uint64_t hash = 1469598103934665603ULL) {
  return str.empty() ? hash
                     : fnv1a(str.substr(1),
                             (hash ^ static_cast<uint64_t>(str.front())) *
                                 1099511628211ULL);
}

}  // namespace fast_little_market