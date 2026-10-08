#include <benchmark/benchmark.h>

#include "orderbook/placeholder.hpp"

// Proves Google Benchmark builds and links. Replace with real benchmarks.
static void BM_Placeholder(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(orderbook::placeholder());
  }
}
BENCHMARK(BM_Placeholder);
