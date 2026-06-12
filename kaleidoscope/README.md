# kaleidoscope

A from-scratch implementation of the [LLVM Kaleidoscope
tutorial](https://llvm.org/docs/tutorial/) in C++23 — lexer, parser, AST, LLVM
IR codegen, and a JIT.

Status: **empty skeleton** — build system only, no compiler yet.

## Requirements

- Clang + libstdc++ 14
- LLVM 20 development package (provides `LLVMConfig.cmake`)

## Build

```sh
make config    # configure (re-run after adding source files)
make build     # compile
make test      # run unit tests (ctest)
make run       # run the REPL
make format    # clang-format in place
```

Sanitizers (off by default):

```sh
make config SANITIZE=address,undefined
make build
```

## Layout

```
include/kaleidoscope/   public headers
src/                    library sources + REPL (main.cpp)
test/                   GoogleTest unit tests
cmake/                  compile flags (strict warnings, -Werror on Linux) + utils
```
