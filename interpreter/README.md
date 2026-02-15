# Monkey Interpreter

A C++23 implementation of the Monkey programming language from Thorsten Ball's [*Writing An Interpreter In Go*](https://interpreterbook.com/).

## Progress

- [x] Chapter 1: Lexing — tokenizer, keywords, two-character tokens, REPL
- [ ] Chapter 2: Parsing — AST, Pratt parser
- [ ] Chapter 3: Evaluation — tree-walking interpreter
- [ ] Chapter 4: Extensions — strings, arrays, hash maps, built-in functions

## Building

```bash
make config    # cmake configure (fetches dependencies on first run)
make build     # build
make test      # run tests
make clean     # clean build artifacts
```

### Prerequisites

- CMake 3.15+
- clang++-18

Dependencies (`fmt`, `GTest`, `magic_enum`) are fetched automatically via CMake FetchContent.

## Project Structure

```
include/monkey/   headers (token.h, lexer.h, repl.h, ...)
src/              implementations + main.cpp
test/             Google Test files
```

Build targets:
- `monkey_lib` — static library (lexer, parser, evaluator, ...)
- `monkey` — REPL executable
- `monkey_test` — test executable
