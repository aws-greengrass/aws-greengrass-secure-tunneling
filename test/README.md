# Testing

Tests use Unity for assertions and CMock for mock generation. Unity is pinned in
the top-level `fc_deps.json` and CMock in `test/fc_deps.json`; both are fetched
at configure time.

## Prerequisites

```bash
sudo apt update && sudo apt install ruby cmake build-essential
```

Ruby drives CMock's generator, and CMake must be 3.22 or newer. `lcov` is only
needed for coverage.

## Running tests

```bash
./test/run_tests.sh
```

Or manually:

```bash
cmake -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
make -C build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Layout

- `unit/` — unit tests, with CMock mocks standing in for dependencies
- `integration/` — tests that link the real implementations
- `test_helpers.c` / `test_helpers.h` — shared helpers, built as the
  `test_helpers` library
- `unit/cmock_config.yml` — CMock generator settings
- `run_tests.sh`, `verify_setup.sh` — convenience scripts

## Coverage

```bash
sudo apt install lcov
./generate_coverage.sh
```

Or manually:

```bash
cmake -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
make -C build -j$(nproc)
ctest --test-dir build
lcov --capture --directory build --output-file build/coverage.info
lcov --remove build/coverage.info '*/test/*' '*/build/_deps/*' \
  --output-file build/coverage_filtered.info
genhtml build/coverage_filtered.info --output-directory build/coverage_html
```

The report lands at `build/coverage_html/index.html`.

## Adding a test

Write the test against Unity:

```c
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

static void test_feature_accepted(void) {
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_feature_accepted);
    return UNITY_END();
}
```

Then register it in `unit/CMakeLists.txt` or `integration/CMakeLists.txt` with
plain CMake — there is no wrapper macro:

```cmake
add_executable(test_<name> ${CMAKE_SOURCE_DIR}/src/<module>.c test_<name>.c)
target_include_directories(
  test_<name> PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(test_<name> PRIVATE "GG_MODULE=(\"test_<name>\")")
target_link_libraries(test_<name> PRIVATE unity test_helpers gg-sdk)
add_test(NAME test_<name> COMMAND test_<name>)
```

Every test defines `GG_MODULE`, which the SDK's logging macros use to label
output.

To mock a dependency, generate the mock in `unit/CMakeLists.txt` and add the
generated source to the executable:

```cmake
generate_mock(${CMAKE_SOURCE_DIR}/src/<dependency>.h)

add_executable(test_<name> ${MOCK_<dependency>_SOURCE} test_<name>.c)
add_dependencies(test_<name> generate_all_mocks)
```

Include `Mock<dependency>.h` from the test, and add `${MOCK_OUTPUT_DIR}` along
with the CMock and Unity source directories to its include path.
`unit/test_secure_tunnel.c` is a worked example.
