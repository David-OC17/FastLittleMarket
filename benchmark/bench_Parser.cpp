#include <benchmark/benchmark.h>

#include <sstream>

#include "Builder.hpp"
#include "Parser.hpp"

namespace flm = fast_little_market;

const std::string sample_new_order_single =
    "8=FIX.5.0^A9=146^A35=D^A34=4^A49=ABC_DEFG01^A52=20090323-15:40:29^A56="
    "CCG^A115=XYZ^A11=NF 0542/03232009^A54=1^A38=100^A55=CVS^A40=1^A59=0^A47="
    "A^A60=20090323-15:40:29^A21=1^A207=N^A10=195^A";

static void BM_Parse(benchmark::State& state) {
  flm::fix::FixMessage msg;
  flm::fix::GroupDefs defs;
  std::istringstream stream(sample_new_order_single);

  for (auto _ : state) {
    stream.seekg(0);  // rewind instead of reconstructing
    flm::fix::FixMessage::parse(stream, msg, defs);
    benchmark::DoNotOptimize(msg.getString(flm::fix::FieldTag::ClOrdID));
  }
}
BENCHMARK(BM_Parse);

static void BM_Build(benchmark::State& state) {
  for (auto _ : state) {
    flm::fix::Builder b;
    b.addField(flm::fix::FieldTag::ClOrdID, std::string_view("NF0542"));
    b.addField(flm::fix::FieldTag::OrderQty, int64_t(100));
    b.addField(flm::fix::FieldTag::Symbol, std::string_view("CVS"));
    b.addField(flm::fix::FieldTag::Side, '1');
    benchmark::DoNotOptimize(b.build());
  }
}
BENCHMARK(BM_Build);

BENCHMARK_MAIN();