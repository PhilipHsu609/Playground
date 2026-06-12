#include "kvstore/version.hpp"

#include <benchmark/benchmark.h>

// NOLINTNEXTLINE(readability-identifier-naming) - Google Benchmark BM_ convention
static void BM_Version(benchmark::State &state) {
    // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores) - benchmark loop idiom
    for (auto _ : state) {
        benchmark::DoNotOptimize(kvstore::version());
    }
}
BENCHMARK(BM_Version);
