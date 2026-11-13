# cgnrc library

[![License: GPL v2](https://img.shields.io/badge/License-GPLv2-blue.svg)](LICENSE) [![Standard: C11](https://img.shields.io/badge/Standard-C11-green.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision)) [![Platform: Linux](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://kernel.org) [![Build: CMake](https://img.shields.io/badge/Build-CMake-blue.svg)](https://cmake.org)

`cgnrc` is a C11 library for building hierarchical command-line interfaces.
It provides module and submodule registration, option parsing and dispatch,
usage rendering, shell completion generation, shell-script execution, and
console/file logging.

## Requirements

- Linux or another POSIX-like system
- CMake 3.14 or newer
- A C11 compiler and pthreads
- Optional: `clang-format`, `clang-tidy`, `cppcheck`, `flawfinder`, Doxygen,
  lcov, and Valgrind for the corresponding helper targets

## Build and test

The static library and tests are enabled by default through CMake's
`BUILD_LIB_A` and `BUILD_TESTING` options. Enable `BUILD_LIB_SO` to also build
the shared variant.

```sh
cmake -S . -B build -DBUILD_LIB_A=ON -DBUILD_LIB_SO=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

For a Release build, use a separate build directory and replace Debug with
Release. Test assertions remain enabled in every configuration. Install with
`cmake --install build --prefix <install-prefix>`.

Run the aggregate static and dynamic analysis suites with
`cmake --build build --target sa` and `cmake --build build --target da`.
Static analysis runs each available tool among cppcheck, Flawfinder, and
clang-tidy. Dynamic analysis runs tests under Valgrind and runs gcov in
isolated `build/valgrind` and `build/gcov` trees, with sanitizers disabled in
those trees to avoid combining incompatible instrumentation.

Installed projects can use `find_package(cgnrc CONFIG REQUIRED)` and link to
`cgnrc::cgnrc_static` and/or `cgnrc::cgnrc_shared`, depending on which library
variants were built and installed. Include `<cgnrc/version.h>` to access the
numeric `CGNRC_VERSION_MAJOR`, `CGNRC_VERSION_MINOR`, and `CGNRC_VERSION_PATCH`
macros and the full `CGNRC_VERSION` string, including `EXTRAVERSION`.

## CLI registry lifecycle

Register the root and all command modules before calling `launch_cli(argv)`.
The framework deep-copies module names, usage strings, and option definitions;
the caller remains responsible for the lifetime of the `argv` strings until
`launch_cli()` returns. `launch_cli()` is single-flight and consumes/destroys
the registered module tree after dispatch, including on parse or callback
failure. Register modules again before a later invocation. Replacing the root
from an action callback is rejected while a launch is active.

Action callbacks receive pointers to framework-owned option records and a
temporary array. Do not free or retain those pointers beyond the callback;
copy values that need to persist. A nonzero callback result stops dispatch
unless `CLI_IGNORE_MODULE_ERRORS` is enabled.

## Shell helpers and logging

Call `generate_sh_completion()` or `generate_bash_completion()` after
registration and before `launch_cli()`, because launching consumes the
registry. `execve_shell_script()` returns the child's exit status, `128 +
signal` for signal termination, or `-1` when process setup, execution, waiting,
or status reporting fails. Logging masks can be configured with
`init_logging_ctx()` and `set_*_log_mask()`.


