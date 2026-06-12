# kvstore

A from-scratch LSM-tree key-value storage engine in C++23.

Status: **empty skeleton** — build system only, no engine yet.

## Build

```sh
make config    # configure (re-run after adding source files)
make build     # compile
make test      # run unit tests (ctest)
make bench     # run benchmarks
make run       # run the CLI
make format    # clang-format in place
```

Sanitizers (off by default):

```sh
make config SANITIZE=address,undefined
make build
```

## Layout

```
include/kvstore/   public headers
src/               library sources + CLI (main.cpp)
test/              GoogleTest unit tests
bench/             Google Benchmark throughput/latency benchmarks
cmake/             compile flags (strict warnings, -Werror on Linux) + utils
```

The `version.*` files are placeholders so the skeleton builds end-to-end;
replace them as real components (memtable, WAL, SSTable, compaction, ...) land.
