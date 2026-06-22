# Building with CMake

## Prerequisites

- A C++20-capable compiler (GCC 11+, Clang 14+, or MSVC 2022).
- CMake ≥ 3.28.
- For the Python package only: Python ≥ 3.8 with development headers
  (e.g. `python3-dev` on Debian/Ubuntu).

## Dependencies

Third-party dependencies (fmt, nlohmann_json, and Catch2 for tests) are fetched
automatically at configure time via CMake's `FetchContent`, so no external
package manager is required. A working internet connection is needed the first
time you configure the project.

## Build

This project doesn't require any special command-line flags to build.

With a single-configuration generator (e.g. Unix Makefiles, Ninja):

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build
```

With a multi-configuration generator (e.g. Visual Studio):

```sh
cmake -S . -B build
cmake --build build --config Release
```

This builds the `parser` command-line executable (see the
[README](README.md#command-line-usage) for its input format):

```sh
./build/parser < request.json
```

### Building with MSVC

MSVC is not standards-compliant by default and needs some flags to behave
properly. See the `flags-msvc` preset in
[CMakePresets.json](CMakePresets.json) for the flags and the variable to provide
them in.

## Tests

Tests use [Catch2][catch2] and require developer mode. They read the example
grammar via a relative path, so run them from the repository root:

```sh
cmake -S . -B build -D parser_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run a single Catch2 test case by tag, and the optional (slow, not part of
`ctest`) benchmark, directly from the repository root:

```sh
./build/test/parser_test "[test_basic]"          # or [test_complex]
./build/test/parser_benchmark "[benchmark]"      # microbenchmark
```

See [HACKING.md](HACKING.md) for the full set of developer-mode targets
(formatting, spell-checking, coverage, docs).

## Python package

The nanobind bindings are packaged with [scikit-build-core][skbuild]. From the
repository root, with Python development headers installed:

```sh
pip install .
```

This builds the compiled extension via CMake and installs the
`korean_verb_parser` package. To build the extension as part of a plain CMake
build instead (e.g. for development), enable `parser_BUILD_PYTHON`:

```sh
cmake -S . -B build -D parser_BUILD_PYTHON=ON -D CMAKE_BUILD_TYPE=Release
cmake --build build --target parser_python
```

## Install

To install the C++ executable (single-configuration generators):

```sh
cmake --install build
```

For multi-configuration generators, specify the configuration:

```sh
cmake --install build --config Release
```

[catch2]: https://github.com/catchorg/Catch2
[skbuild]: https://scikit-build-core.readthedocs.io/
