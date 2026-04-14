#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace FastLittleMarket {
namespace FIX {

#define MSGTYPE_LIST(X)                            \
  X(Heartbeat, "0")                                \
  X(TestRequest, "1")                              \
  X(ResendRequest, "2")                            \
  X(Reject, "3")                                   \
  X(SequenceReset, "4")                            \
  X(Logout, "5")                                   \
  X(IOI, "6")                                      \
  X(Advertisement, "7")                            \
  X(ExecutionReport, "8")                          \
  X(OrderCancelReject, "9")                        \
  X(QuoteStatusRequest, "a")                       \
  X(Logon, "A")                                    \
  X(DerivativeSecurityList, "AA")                  \
  X(NewOrderMultileg, "AB")                        \
  X(MultilegOrderCancelReplace, "AC")              \
  X(TradeCaptureReportRequest, "AD")               \
  X(TradeCaptureReport, "AE")                      \
  X(OrderMassStatusRequest, "AF")                  \
  X(QuoteRequestReject, "AG")                      \
  X(RFQRequest, "AH")                              \
  X(QuoteStatusReport, "AI")                       \
  X(QuoteResponse, "AJ")                           \
  X(Confirmation, "AK")                            \
  X(PositionMaintenanceRequest, "AL")              \
  X(PositionMaintenanceReport, "AM")               \
  X(RequestForPositions, "AN")                     \
  X(RequestForPositionsAck, "AO")                  \
  X(PositionReport, "AP")                          \
  X(TradeCaptureReportRequestAck, "AQ")            \
  X(TradeCaptureReportAck, "AR")                   \
  X(AllocationReport, "AS")                        \
  X(AllocationReportAck, "AT")                     \
  X(ConfirmationAck, "AU")                         \
  X(SettlementInstructionRequest, "AV")            \
  X(AssignmentReport, "AW")                        \
  X(CollateralRequest, "AX")                       \
  X(CollateralAssignment, "AY")                    \
  X(CollateralResponse, "AZ")                      \
  X(MassQuoteAcknowledgement, "b")                 \
  X(News, "B")                                     \
  X(CollateralReport, "BA")                        \
  X(CollateralInquiry, "BB")                       \
  X(NetworkCounterpartySystemStatusRequest, "BC")  \
  X(NetworkCounterpartySystemStatusResponse, "BD") \
  X(UserRequest, "BE")                             \
  X(UserResponse, "BF")                            \
  X(CollateralInquiryAck, "BG")                    \
  X(ConfirmationRequest, "BH")                     \
  X(TradingSessionListRequest, "BI")               \
  X(TradingSessionList, "BJ")                      \
  X(SecurityListUpdateReport, "BK")                \
  X(AdjustedPositionReport, "BL")                  \
  X(AllocationInstructionAlert, "BM")              \
  X(ExecutionAcknowledgement, "BN")                \
  X(ContraryIntentionReport, "BO")                 \
  X(SecurityDefinitionUpdateReport, "BP")          \
  X(SecurityDefinitionRequest, "c")                \
  X(Email, "C")                                    \
  X(SecurityDefinition, "d")                       \
  X(NewOrderSingle, "D")                           \
  X(SecurityStatusRequest, "e")                    \
  X(NewOrderList, "E")                             \
  X(SecurityStatus, "f")                           \
  X(OrderCancelRequest, "F")                       \
  X(TradingSessionStatusRequest, "g")              \
  X(OrderCancelReplaceRequest, "G")                \
  X(TradingSessionStatus, "h")                     \
  X(OrderStatusRequest, "H")                       \
  X(MassQuote, "i")                                \
  X(BusinessMessageReject, "j")                    \
  X(AllocationInstruction, "J")                    \
  X(BidRequest, "k")                               \
  X(ListCancelRequest, "K")                        \
  X(BidResponse, "l")                              \
  X(ListExecute, "L")                              \
  X(ListStrikePrice, "m")                          \
  X(ListStatusRequest, "M")                        \
  X(XML_non_FIX, "n")                              \
  X(ListStatus, "N")                               \
  X(RegistrationInstructions, "o")                 \
  X(RegistrationInstructionsResponse, "p")         \
  X(AllocationInstructionAck, "P")                 \
  X(OrderMassCancelRequest, "q")                   \
  X(DontKnowTradeDK, "Q")                          \
  X(OrderMassCancelReport, "r")                    \
  X(QuoteRequest, "R")                             \
  X(NewOrderCross, "s")                            \
  X(Quote, "S")                                    \
  X(CrossOrderCancelReplaceRequest, "t")           \
  X(SettlementInstructions, "T")                   \
  X(CrossOrderCancelRequest, "u")                  \
  X(SecurityTypeRequest, "v")                      \
  X(MarketDataRequest, "V")                        \
  X(SecurityTypes, "w")                            \
  X(MarketDataSnapshotFullRefresh, "W")            \
  X(SecurityListRequest, "x")                      \
  X(MarketDataIncrementalRefresh, "X")             \
  X(SecurityList, "y")                             \
  X(MarketDataRequestReject, "Y")                  \
  X(DerivativeSecurityListRequest, "z")            \
  X(QuoteCancel, "Z")

enum class MsgType : uint16_t {
#define X(name, val) name,
  MSGTYPE_LIST(X)
#undef X
      _COUNT
};

constexpr size_t MSGTYPE_COUNT = static_cast<size_t>(MsgType::_COUNT);

constexpr std::array<std::string_view, MSGTYPE_COUNT> msgtype_to_fix = {
#define X(name, val) val,
    MSGTYPE_LIST(X)
#undef X
};

constexpr std::array<std::string_view, MSGTYPE_COUNT> msgtype_to_name = {
#define X(name, val) #name,
    MSGTYPE_LIST(X)
#undef X
};

constexpr std::optional<MsgType> stom(std::string_view s) noexcept {
  for (size_t i = 0; i < MSGTYPE_COUNT; ++i) {
    if (msgtype_to_fix[i] == s) {
      return static_cast<MsgType>(i);
    }
  }
  return std::nullopt;
}

constexpr std::string_view mtos(MsgType t) noexcept {
  return msgtype_to_name[static_cast<size_t>(t)];
}

constexpr std::string_view mtof(MsgType t) noexcept {
  return msgtype_to_fix[static_cast<size_t>(t)];
}

static_assert(msgtype_to_fix.size() == msgtype_to_name.size());

namespace {

constexpr uint64_t MSGTYPE_LIST_HASH = 26629920070445379ULL;

constexpr uint64_t fnv1a(const std::string_view str,
                         uint64_t hash = 1469598103934665603ULL) {
  return str.empty() ? hash
                     : fnv1a(str.substr(1),
                             (hash ^ static_cast<uint64_t>(str.front())) *
                                 1099511628211ULL);
}

constexpr uint64_t compute_msgtype_hash() {
  uint64_t h = 1469598103934665603ULL;

#define X(name, val)   \
  h = fnv1a(#name, h); \
  h = fnv1a(val, h);   \
  h *= 1099511628211ULL;

  MSGTYPE_LIST(X)

#undef X

  return h;
}

static_assert(compute_msgtype_hash() == MSGTYPE_LIST_HASH,
              "MsgType definition changed!");

}  // namespace

}  // namespace FIX
}  // namespace FastLittleMarket