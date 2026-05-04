#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "CommonTools.hpp"

namespace fast_little_market {
namespace fix {

#define SOH "\x01"

// FIX 5.0 field tags (tag number as string, name as enum label).
#define FIELDTAG_LIST(X)                 \
  X(Account, "1")                        \
  X(AdvId, "2")                          \
  X(AdvRefID, "3")                       \
  X(AdvSide, "4")                        \
  X(AdvTransType, "5")                   \
  X(AvgPx, "6")                          \
  X(BeginSeqNo, "7")                     \
  X(BeginString, "8")                    \
  X(BodyLength, "9")                     \
  X(CheckSum, "10")                      \
  X(ClOrdID, "11")                       \
  X(Commission, "12")                    \
  X(CommType, "13")                      \
  X(CumQty, "14")                        \
  X(Currency, "15")                      \
  X(EndSeqNo, "16")                      \
  X(ExecID, "17")                        \
  X(ExecInst, "18")                      \
  X(ExecRefID, "19")                     \
  X(HandlInst, "21")                     \
  X(SecurityIDSource, "22")              \
  X(IOIID, "23")                         \
  X(IOIQltyInd, "25")                    \
  X(IOIRefID, "26")                      \
  X(IOIQty, "27")                        \
  X(IOITransType, "28")                  \
  X(LastCapacity, "29")                  \
  X(LastMkt, "30")                       \
  X(LastPx, "31")                        \
  X(LastQty, "32")                       \
  X(NoLinesOfText, "33")                 \
  X(MsgSeqNum, "34")                     \
  X(MsgType, "35")                       \
  X(NewSeqNo, "36")                      \
  X(OrderID, "37")                       \
  X(OrderQty, "38")                      \
  X(OrdStatus, "39")                     \
  X(OrdType, "40")                       \
  X(OrigClOrdID, "41")                   \
  X(OrigTime, "42")                      \
  X(PossDupFlag, "43")                   \
  X(Price, "44")                         \
  X(RefSeqNum, "45")                     \
  X(SecurityID, "48")                    \
  X(SenderCompID, "49")                  \
  X(SenderSubID, "50")                   \
  X(SendingTime, "52")                   \
  X(Quantity, "53")                      \
  X(Side, "54")                          \
  X(Symbol, "55")                        \
  X(TargetCompID, "56")                  \
  X(TargetSubID, "57")                   \
  X(Text, "58")                          \
  X(TimeInForce, "59")                   \
  X(TransactTime, "60")                  \
  X(Urgency, "61")                       \
  X(ValidUntilTime, "62")                \
  X(SettlType, "63")                     \
  X(SettlDate, "64")                     \
  X(SymbolSfx, "65")                     \
  X(ListID, "66")                        \
  X(ListSeqNo, "67")                     \
  X(TotNoOrders, "68")                   \
  X(ListExecInst, "69")                  \
  X(AllocID, "70")                       \
  X(AllocTransType, "71")                \
  X(RefAllocID, "72")                    \
  X(NoOrders, "73")                      \
  X(AvgPxPrecision, "74")                \
  X(TradeDate, "75")                     \
  X(PositionEffect, "77")                \
  X(NoAllocs, "78")                      \
  X(AllocAccount, "79")                  \
  X(AllocShares, "80")                   \
  X(ProcessCode, "81")                   \
  X(NoRpts, "82")                        \
  X(RptSeq, "83")                        \
  X(CxlQty, "84")                        \
  X(NoDlvyInst, "85")                    \
  X(AllocStatus, "87")                   \
  X(AllocRejCode, "88")                  \
  X(Signature, "89")                     \
  X(SecureDataLen, "90")                 \
  X(SecureData, "91")                    \
  X(SignatureLength, "93")               \
  X(EmailType, "94")                     \
  X(RawDataLength, "95")                 \
  X(RawData, "96")                       \
  X(PossResend, "97")                    \
  X(EncryptMethod, "98")                 \
  X(StopPx, "99")                        \
  X(ExDestination, "100")                \
  X(CxlRejReason, "102")                 \
  X(OrdRejReason, "103")                 \
  X(IOIQualifier, "104")                 \
  X(Issuer, "106")                       \
  X(SecurityDesc, "107")                 \
  X(HeartBtInt, "108")                   \
  X(ClientID, "109")                     \
  X(MinQty, "110")                       \
  X(MaxFloor, "111")                     \
  X(TestReqID, "112")                    \
  X(ReportToExch, "113")                 \
  X(LocateReqd, "114")                   \
  X(OnBehalfOfCompID, "115")             \
  X(OnBehalfOfSubID, "116")              \
  X(QuoteID, "117")                      \
  X(NetMoney, "118")                     \
  X(SettlCurrAmt, "119")                 \
  X(SettlCurrency, "120")                \
  X(ForexReq, "121")                     \
  X(OrigSendingTime, "122")              \
  X(GapFillFlag, "123")                  \
  X(NoExecs, "124")                      \
  X(ExpireTime, "126")                   \
  X(DKReason, "127")                     \
  X(DeliverToCompID, "128")              \
  X(DeliverToSubID, "129")               \
  X(IOINaturalFlag, "130")               \
  X(QuoteReqID, "131")                   \
  X(BidPx, "132")                        \
  X(OfferPx, "133")                      \
  X(BidSize, "134")                      \
  X(OfferSize, "135")                    \
  X(NoMiscFees, "136")                   \
  X(MiscFeeAmt, "137")                   \
  X(MiscFeeCurr, "138")                  \
  X(MiscFeeType, "139")                  \
  X(PrevClosePx, "140")                  \
  X(ResetSeqNumFlag, "141")              \
  X(SenderLocationID, "142")             \
  X(TargetLocationID, "143")             \
  X(OnBehalfOfLocationID, "144")         \
  X(DeliverToLocationID, "145")          \
  X(NoRelatedSym, "146")                 \
  X(Subject, "147")                      \
  X(Headline, "148")                     \
  X(URLLink, "149")                      \
  X(ExecType, "150")                     \
  X(LeavesQty, "151")                    \
  X(CashOrderQty, "152")                 \
  X(AllocAvgPx, "153")                   \
  X(AllocNetMoney, "154")                \
  X(SettlCurrFxRate, "155")              \
  X(SettlCurrFxRateCalc, "156")          \
  X(NumDaysInterest, "157")              \
  X(AccruedInterestRate, "158")          \
  X(AccruedInterestAmt, "159")           \
  X(SettlInstMode, "160")                \
  X(AllocText, "161")                    \
  X(SettlInstID, "162")                  \
  X(SettlInstTransType, "163")           \
  X(EmailThreadID, "164")                \
  X(SettlInstSource, "165")              \
  X(SecurityType, "167")                 \
  X(EffectiveTime, "168")                \
  X(StandInstDbType, "169")              \
  X(StandInstDbName, "170")              \
  X(StandInstDbID, "171")                \
  X(SettlDeliveryType, "172")            \
  X(BidSpotRate, "188")                  \
  X(BidForwardPoints, "189")             \
  X(OfferSpotRate, "190")                \
  X(OfferForwardPoints, "191")           \
  X(OrderQty2, "192")                    \
  X(SettlDate2, "193")                   \
  X(LastSpotRate, "194")                 \
  X(LastForwardPoints, "195")            \
  X(AllocLinkID, "196")                  \
  X(AllocLinkType, "197")                \
  X(SecondaryOrderID, "198")             \
  X(NoIOIQualifiers, "199")              \
  X(MaturityMonthYear, "200")            \
  X(PutOrCall, "201")                    \
  X(StrikePrice, "202")                  \
  X(CoveredOrUncovered, "203")           \
  X(OptAttribute, "206")                 \
  X(SecurityExchange, "207")             \
  X(NotifyBrokerOfCredit, "208")         \
  X(AllocHandlInst, "209")               \
  X(MaxShow, "210")                      \
  X(PegOffsetValue, "211")               \
  X(XmlDataLen, "212")                   \
  X(XmlData, "213")                      \
  X(SettlInstRefID, "214")               \
  X(NoRoutingIDs, "215")                 \
  X(RoutingType, "216")                  \
  X(RoutingID, "217")                    \
  X(Spread, "218")                       \
  X(BenchmarkCurveCurrency, "220")       \
  X(BenchmarkCurveName, "221")           \
  X(BenchmarkCurvePoint, "222")          \
  X(CouponRate, "223")                   \
  X(CouponPaymentDate, "224")            \
  X(IssueDate, "225")                    \
  X(RepurchaseTerm, "226")               \
  X(RepurchaseRate, "227")               \
  X(Factor, "228")                       \
  X(TradeOriginationDate, "229")         \
  X(ExDate, "230")                       \
  X(ContractMultiplier, "231")           \
  X(NoStipulations, "232")               \
  X(StipulationType, "233")              \
  X(StipulationValue, "234")             \
  X(YieldType, "235")                    \
  X(Yield, "236")                        \
  X(TotalTakedown, "237")                \
  X(Concession, "238")                   \
  X(RepoCollateralSecurityType, "239")   \
  X(RedemptionDate, "240")               \
  X(UnderlyingCouponRate, "245")         \
  X(UnderlyingContractMultiplier, "246") \
  X(UnderlyingSecurityID, "309")         \
  X(UnderlyingSecurityIDSource, "305")   \
  X(UnderlyingSymbol, "311")             \
  X(TradingSessionID, "336")             \
  X(NoLegs, "555")                       \
  X(LegCurrency, "556")                  \
  X(LegPrice, "566")                     \
  X(LegSymbol, "600")                    \
  X(LegSecurityID, "602")                \
  X(LegSecurityIDSource, "603")          \
  X(TradingSessionSubID, "625")          \
  X(LegQty, "687")                       \
  X(LegOrderQty, "685")                  \
  X(LegRatioQty, "623")                  \
  X(LegSide, "624")                      \
  X(NoUnderlyings, "711")                \
  X(UnderlyingPx, "810")                 \
  X(UnderlyingQty, "879")                \
  X(INVALID, "\0")

enum class FieldTag : uint16_t {
#define X(name, val) name,
  FIELDTAG_LIST(X)
#undef X
      COUNT
};

constexpr size_t FIELDTAG_COUNT = static_cast<size_t>(FieldTag::COUNT);

constexpr std::array<std::string_view, FIELDTAG_COUNT> FIELDTAG_TO_FIX = {
#define X(name, val) val,
    FIELDTAG_LIST(X)
#undef X
};

constexpr std::array<std::string_view, FIELDTAG_COUNT> FIELDTAG_TO_NAME = {
#define X(name, val) #name,
    FIELDTAG_LIST(X)
#undef X
};

static_assert(FIELDTAG_TO_FIX.size() == FIELDTAG_TO_NAME.size());

// FIX tag number string → FieldTag
constexpr std::optional<FieldTag> stoft(std::string_view s) noexcept {
  for (size_t i = 0; i < FIELDTAG_COUNT; ++i) {
    if (FIELDTAG_TO_FIX[i] == s) {
      return static_cast<FieldTag>(i);
    }
  }
  return std::nullopt;
}

constexpr std::string_view fton(FieldTag t) noexcept {
  return FIELDTAG_TO_NAME[static_cast<size_t>(t)];
}

constexpr std::string_view ftof(FieldTag t) noexcept {
  return FIELDTAG_TO_FIX[static_cast<size_t>(t)];
}

namespace {

constexpr uint64_t FIELDTAG_LIST_HASH = 1622422181734463442ULL;

constexpr uint64_t computeFieldtagHash() {
  uint64_t h = 1469598103934665603ULL;

#define X(name, val)   \
  h = fnv1a(#name, h); \
  h = fnv1a(val, h);   \
  h *= 1099511628211ULL;

  FIELDTAG_LIST(X)

#undef X

  return h;
}

static_assert(computeFieldtagHash() == FIELDTAG_LIST_HASH,
              "FieldTag definition changed!");

}  // namespace

}  // namespace fix
}  // namespace fast_little_market