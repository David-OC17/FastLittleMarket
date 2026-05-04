#pragma once

#include "ExchangeLogger.hpp"
#include "Order.hpp"
#include "quill/LogMacros.h"

namespace fast_little_market {

#define LOG_INFO(...) \
  QUILL_LOG_INFO(fast_little_market::global_logger, __VA_ARGS__)
#define LOG_WARNING(...) \
  QUILL_LOG_WARNING(fast_little_market::global_logger, __VA_ARGS__)
#define LOG_ERROR(...) \
  QUILL_LOG_ERROR(fast_little_market::global_logger, __VA_ARGS__)

#define FLM_ORDER_ID(order) ((order).id_ns_ >> 32)
#define FLM_ORDER_TS(order) ((order).id_ns_ & 0xFFFFFFFFULL)
#define FLM_SIDE_STR(order) \
  ((order).side_ == fast_little_market::BUY_SIDE ? "BUY" : "SELL")
#define FLM_TYPE_STR(order) \
  ((order).type_ == fast_little_market::MARKET_TYPE ? "MKT" : "LMT")
#define FLM_TIF_STR(order)                               \
  ((order).tim_ == fast_little_market::DAY_TIF   ? "DAY" \
   : (order).tim_ == fast_little_market::GTC_TIF ? "GTC" \
                                                 : "IOC")

#define LOG_NEW_ORDER_SINGLE(order)                                      \
  QUILL_LOG_INFO(                                                        \
      fast_little_market::global_logger,                                 \
      "NewOrderSingle  id={} ts={} client={:<8} side={} type={} tif={} " \
      "price_q4={} vol={}",                                              \
      FLM_ORDER_ID(order), FLM_ORDER_TS(order), (order).client_,         \
      FLM_SIDE_STR(order), FLM_TYPE_STR(order), FLM_TIF_STR(order),      \
      (order).price_q4_, (order).volume_)

#define LOG_CANCEL_ORDER_REQUEST(order)                                   \
  QUILL_LOG_INFO(                                                         \
      fast_little_market::global_logger,                                  \
      "CancelOrderRequest  id={} ts={} client={:<8} side={} price_q4={} " \
      "vol={}",                                                           \
      FLM_ORDER_ID(order), FLM_ORDER_TS(order), (order).client_,          \
      FLM_SIDE_STR(order), (order).price_q4_, (order).volume_)

#define LOG_MATCH(passive, aggressive, fill_vol, fill_price_q4)             \
  QUILL_LOG_INFO(                                                           \
      fast_little_market::global_logger,                                    \
      "Match  passive_id={} aggressive_id={} fill_vol={} fill_price_q4={}", \
      FLM_ORDER_ID(passive), FLM_ORDER_ID(aggressive), (fill_vol),          \
      (fill_price_q4))

#define LOG_ORDER_ACCEPTED(order)                                         \
  QUILL_LOG_INFO(                                                         \
      fast_little_market::global_logger,                                  \
      "OrderAccepted   id={} ts={} client={:<8} side={} type_={} tif={} " \
      "price_q4={} vol={}",                                               \
      FLM_ORDER_ID(order), FLM_ORDER_TS(order), (order).client_,          \
      FLM_SIDE_STR(order), FLM_TYPE_STR(order), FLM_TIF_STR(order),       \
      (order).price_q4_, (order).volume_)

#define LOG_ORDER_REJECTED(order, reason)                                   \
  QUILL_LOG_INFO(fast_little_market::global_logger,                         \
                 "OrderRejected   id={} ts={} client={:<8} reason={}",      \
                 FLM_ORDER_ID(order), FLM_ORDER_TS(order), (order).client_, \
                 (reason))

#define LOG_ORDER_REPLACED(old_order, new_order)                          \
  QUILL_LOG_INFO(fast_little_market::global_logger,                       \
                 "OrderReplaced   old_id={} new_id={} client={:<8} "      \
                 "old_price_q4={} new_price_q4={} old_vol={} new_vol={}", \
                 FLM_ORDER_ID(old_order), FLM_ORDER_ID(new_order),        \
                 (old_order).client_, (old_order).price_q4_,              \
                 (new_order).price_q4_, (old_order).volume_,              \
                 (new_order).volume_)

#define LOG_ORDER_CANCELED(order)                                            \
  QUILL_LOG_INFO(                                                            \
      fast_little_market::global_logger,                                     \
      "OrderCanceled   id={} ts={} client={:<8} side={} price_q4={} vol={}", \
      FLM_ORDER_ID(order), FLM_ORDER_TS(order), (order).client_,             \
      FLM_SIDE_STR(order), (order).price_q4_, (order).volume_)

// fill_vol    = quantity matched in this execution
// leaves_vol  = remaining quantity on the order after this fill
#define LOG_ORDER_EXECUTED(order, fill_vol, leaves_vol, fill_price_q4)      \
  QUILL_LOG_INFO(fast_little_market::global_logger,                         \
                 "OrderExecuted   id={} ts={} client={:<8} side={} "        \
                 "fill_vol={} leaves_vol={} fill_price_q4={}",              \
                 FLM_ORDER_ID(order), FLM_ORDER_TS(order), (order).client_, \
                 FLM_SIDE_STR(order), (fill_vol), (leaves_vol),             \
                 (fill_price_q4))

}  // namespace fast_little_market