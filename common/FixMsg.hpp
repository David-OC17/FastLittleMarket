#pragma once

#include <array>
#include <string_view>

namespace FastLittleMarket {
namespace FIX {

#include <array>
#include <string_view>

#define MSGTYPE_LIST(X) \
  X(Heartbeat, '0')     \
  X(TestRequest, '1')   \
  X(ResendRequest, '2') \
  X(Reject, '3')        \
  X(SequenceReset, '4') \
  X(Logout, '5') \
  X(IndicationOfInterest, '6') \
  X(Advertisement, '7') \
  X(ExecutionReport, '8') \
  X(OrderCancelReject, '9') \
  X(QuoteStatusRequest, 'a') \
  X(Logon, 'A') \
  X(QuoteAcknowledgement, 'b') \
  X(News, 'B') \
  X(SecurityDefinitionRequest, 'c') \
  X(Email, 'C') \
  X(SecurityDefinition, 'd') \
  X(NewOrderSingle, 'D') \
  X(SecurityStatusRequest, 'e') \
  X(NewOrderList, 'E') \
  X(SecurityStatus, 'f') \
  X(OrderCancelRequest, 'F') \
  X(TradingSessionStatusRequest, 'g') \
  X(OrderCancelReplaceRequest, 'G') \
  X(TradingSessionStatus, 'h') \
  X(OrderStatusRequest, 'H') \
  X(MassQuote, 'i') \
  X(BusinessMessageReject, 'j') \
  X(Allocation, 'J') \
  X(BidRequest, 'k') \
  X(ListCancelRequest, 'K') \
  X(BidResponse, 'l') \
  X(ListExecute, 'L') \
  X(ListStrikePrice, 'm') \
  X(ListStatusRequest, 'M') \
  X(ListStatus, 'N') \
  X(AllocationAck, 'P') \
  X(DontKnowTrade, 'Q') \
  X(QuoteRequest, 'R') \
  X(Quote, 'S') \
  X(SettlementInstructions, 'T') \
  X(MarketDataRequest, 'V') \
  X(MarketDataSnapshotFullRefresh, 'W') \
  X(MarketDataIncrementalRefresh, 'X') \
  X(MarketDataRequestReject, 'Y') \
  X(QuoteCancel, 'Z')

enum class MsgType : char {
#define X(name, val) name = val,
  MSGTYPE_LIST(X)
#undef X
};

constexpr std::optional<MsgType> stom(std::string_view s) noexcept {
#define X(name, val) \
  if (s == #name) return MsgType::name;
  MSGTYPE_LIST(X)
#undef X
  return std::nullopt;
}

constexpr std::string_view mtos(MsgType t) noexcept {
  switch (t) {
#define X(name, val)  \
  case MsgType::name: \
    return #name;
    MSGTYPE_LIST(X)
#undef X
  }
  std::unreachable();
}

namespace {

constexpr uint64_t MSGTYPE_LIST_HASH = 17642892922276368429ULL;

constexpr uint64_t fnv1a(const char* str, uint64_t hash = 1469598103934665603ULL) {
  return (*str == 0)
    ? hash
    : fnv1a(str + 1, (hash ^ static_cast<uint64_t>(*str)) * 1099511628211ULL);
}

constexpr uint64_t compute_msgtype_hash() {
  uint64_t h = 1469598103934665603ULL;

#define X(name, val) \
  h = fnv1a(#name, h); \
  h ^= static_cast<uint64_t>(val); \
  h *= 1099511628211ULL;

  MSGTYPE_LIST(X)

#undef X

  return h;
}

static_assert(compute_msgtype_hash() == MSGTYPE_LIST_HASH,
              "MsgType definition changed!");

}

}  // namespace FIX
}  // namespace FastLittleMarket
