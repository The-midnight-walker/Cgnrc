## Development setup

Configure a Debug build with both library variants and tests enabled:

```sh
cmake -S . -B build -DBUILD_LIB_A=ON -DBUILD_LIB_SO=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The project treats compiler warnings as errors. Keep public headers
self-contained, document API ownership and lifecycle constraints, and add
regression tests for bug fixes. Comments and documentation should be written
in English.

## Before submitting

- Run the complete test suite in Debug and Release configurations.
- Run the `format` target when `clang-format` is available.
- Review the diff for generated files, temporary artifacts, and unrelated
	changes.
- Include a concise summary of behavior changes and compatibility impacts.
