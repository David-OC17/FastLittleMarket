#pragma once

#include <array>
#include <string>

namespace fast_little_market {

// TODO: Load symbols from a config file or database
static std::array<std::string, 100> symbols = {
    "AAPL",   "MSFT",     "GOOGL",  "AMZN",   "META",    "TSLA",   "NVDA",
    "BRK.A",  "JPM",      "JNJ",    "V",      "PG",      "UNH",    "HD",
    "MA",     "DIS",      "BAC",    "XOM",    "PFE",     "KO",     "PEP",
    "ABBV",   "MRK",      "COST",   "AVGO",   "TMO",     "CSCO",   "WMT",
    "ABT",    "CRM",      "ACN",    "DHR",    "NEE",     "VZ",     "ADBE",
    "CMCSA",  "NFLX",     "INTC",   "AMD",    "QCOM",    "TXN",    "HON",
    "LIN",    "ORCL",     "UPS",    "LOW",    "PM",      "IBM",    "RTX",
    "CVX",    "GS",       "MS",     "BLK",    "SCHW",    "SPGI",   "DE",
    "CAT",    "GE",       "BA",     "F",      "GM",      "UBER",   "LYFT",
    "SNAP",   "SHOP",     "SQ",     "PYPL",   "COIN",    "ROKU",   "ZM",
    "BTCUSD", "ETHUSD",   "SOLUSD", "XRPUSD", "DOGEUSD", "BNBUSD", "ADAUSD",
    "DOTUSD", "MATICUSD", "LTCUSD", "EURUSD", "USDJPY",  "GBPUSD", "AUDUSD",
    "USDCAD", "USDCHF",   "NZDUSD", "EURJPY", "GBPJPY",  "EURGBP", "XAUUSD",
    "XAGUSD", "CL",       "NG",     "GC",     "SI",      "HG",     "PL",
    "PA",     "ZC"};

}  // namespace fast_little_market