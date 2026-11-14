# cgnrc library

[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](LICENSE) [![Standard: C11](https://img.shields.io/badge/Standard-C11-green.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision)) [![Platform: Linux](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://kernel.org) [![Build: CMake](https://img.shields.io/badge/Build-CMake-blue.svg)](https://cmake.org)

`cgnrc` is a C11 library for hierarchical command-line interfaces, shell
completion, script execution, and logging.

## Requirements

- Linux or another POSIX-like system
- CMake 3.14 or newer
- A C11 compiler and pthreads
- Required for configuration and installation: Doxygen
- Optional: `clang-format`, `clang-tidy`, `cppcheck`, `flawfinder`, lcov, and
    Valgrind for the corresponding helper targets

## Build and test

Build the static library and tests by default; set `BUILD_LIB_SO=ON` for the
shared library.

```sh
cmake -S . -B build -DBUILD_LIB_A=ON -DBUILD_LIB_SO=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Install with a Release build using `cmake --build build --target install`.
Libraries, headers, generated HTML documentation, and CMake package files are
placed under `build/cgnrc/`; libraries are stripped during installation.
The documentation is generated automatically by the install target, or can be
generated separately with `cmake --build build --target doc`.

Run static and dynamic analysis with `cmake --build build --target sa` and
`cmake --build build --target da`. Coverage reports are generated under
`build/gcov/` and `build/lcov/`.

Consumers can use `find_package(cgnrc CONFIG REQUIRED)` and link against
`cgnrc::cgnrc_static` or `cgnrc::cgnrc_shared`. Version macros are available
from `<cgnrc/version.h>`.

## CLI registry lifecycle

Register modules before `launch_cli(argv)`. Each launch consumes the registry;
register modules again before another invocation. Callback option pointers are
temporary and must not be retained or freed.

## Shell helpers and logging

Generate completions after registration and before launch. The shell helper
returns the child status, or `-1` on execution errors. Configure logging with
`init_logging_ctx()` and the log-mask setters.


