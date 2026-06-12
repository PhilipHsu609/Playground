EXE = kaleidoscope
BUILD_TYPE ?= Debug
BUILD_DIR = build

# Optional sanitizers, e.g.: make config SANITIZE=address,undefined
SANITIZE ?=

# Where find_package(LLVM) looks; from the installed llvm-config.
LLVM_CONFIG ?= llvm-config-20
LLVM_DIR := $(shell $(LLVM_CONFIG) --cmakedir)

CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
              -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang \
              -DSANITIZE=$(SANITIZE) \
              -DLLVM_DIR=$(LLVM_DIR)

.PHONY: all config format build test run clean

all: format build test

format:
	find include src test \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' \) | xargs clang-format -i

config:
	cmake $(CMAKE_FLAGS) -S . -B $(BUILD_DIR)

build:
	cmake --build $(BUILD_DIR) --parallel

test:
	cd $(BUILD_DIR)/test && ctest --output-on-failure

run: build
	./$(BUILD_DIR)/src/$(EXE)

clean:
	cmake --build $(BUILD_DIR) --target clean
