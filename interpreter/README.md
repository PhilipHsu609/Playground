# C++ Project Template

[![CI](https://github.com/PhilipHsu609/CppTemplate/actions/workflows/ci.yml/badge.svg)](https://github.com/PhilipHsu609/CppTemplate/actions/workflows/ci.yml)

A modern, easy-to-use C++ project template designed for small to personal projects. Focus on simplicity and extensibility.

## Features

- Modern CMake (3.15+) build system
- vcpkg for dependency management
- GoogleTest for unit testing
- Clang-format and clang-tidy for code quality
- Pre-configured compiler warnings and flags
- Simple Makefile wrapper for common tasks
- Automated CI/CD with GitHub Actions

## Quick Start

### Prerequisites

- CMake 3.15 or higher
- A C++17 compatible compiler (GCC, Clang, or MSVC)
- [vcpkg](https://github.com/microsoft/vcpkg) package manager

### Setup

1. Clone this template repository
2. Set the `VCPKG_ROOT` environment variable to your vcpkg installation path:
   ```bash
   export VCPKG_ROOT=/path/to/vcpkg
   ```
3. Modify `CMakeLists.txt` to set your project name (replace `MyProject`)
4. Update `vcpkg.json` if you need different dependencies

### Building

Using the Makefile (Linux/macOS):
```bash
make all          # Configure, build, and test
make build        # Build only
make test         # Run tests
make clean        # Clean build artifacts
```

Or using CMake directly:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest
```

## Folder Structure

- `include/`: Public header files (organized by project name)
- `src/`: Source files and private headers
- `test/`: Unit test files
- `cmake/`: CMake modules and utility scripts
- `bin/`: Output binaries (generated)
- `build/`: Build artifacts (generated)

## Optional Folders (Create as Needed)

- `doc/`: Documentation
- `examples/`: Usage examples
- `third_party/`: Third-party libraries (non-vcpkg)
- `data/`: Data files
- `utils/`: Utility scripts

## File Extension Conventions

- **Headers**: `.h`, `.hpp`, `.hxx`, `.hh`
- **Header implementations**: `.i.h`, `.i.hpp`, `-inl.h`, `-inl.hpp`
- **Sources**: `.c`, `.cpp`, `.cxx`, `.cc`

## Customization

### Adding Dependencies

Edit `vcpkg.json` to add new dependencies:
```json
{
  "dependencies": [
    "fmt",
    "gtest",
    "your-new-dependency"
  ]
}
```

The `vcpkg-configuration.json` file pins the vcpkg baseline version to ensure reproducible builds. Update the baseline commit hash periodically to get newer package versions.

### Compiler Flags

Modify `cmake/CompileFlags.cmake` to adjust warning levels and compiler options.

### Code Style

- `.clang-format`: Code formatting rules (based on LLVM style)
- `.clang-tidy`: Static analysis rules

Run formatting:
```bash
clang-format -i src/*.cpp include/**/*.hpp
```

## Continuous Integration

The project includes a GitHub Actions workflow (`.github/workflows/ci.yml`) that:
- Builds the project on Linux, macOS, and Windows
- Tests with multiple compilers (GCC, Clang, MSVC)
- Runs all unit tests automatically
- Uploads test results on failure

The CI runs on every push and pull request to the `main` branch.

## Project Structure Guidelines

1. Keep public headers in `include/<project-name>/`
2. Keep implementation details in `src/`
3. One test file per source file in `test/`
4. Use descriptive names for executables and libraries

## License

This template is provided under the MIT License. See LICENSE file for details.
