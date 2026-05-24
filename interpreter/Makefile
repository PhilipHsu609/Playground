EXE = monkey
BUILD_TYPE ?= Debug
BUILD_DIR = build

CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
              -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

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
