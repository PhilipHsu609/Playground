# Benchmark executable
set(BENCH_TARGET kvstore_bench)

add_executable(${BENCH_TARGET})

target_sources(
    ${BENCH_TARGET}
    PRIVATE
    version_bench.cpp
)

target_link_libraries(
    ${BENCH_TARGET}
    PRIVATE
    kvstore_lib
    benchmark::benchmark_main
)

target_compile_features(${BENCH_TARGET} PRIVATE cxx_std_23)
