# Testing Guide

This guide explains how to run existing tests and add new tests to the project.

## Test Frameworks

- **Unity** (v2.6.1) - Assertions and test runner
- **CMock** (v2.6.0) - Mock generation (automatic from headers)

## Prerequisites

Install dependencies:

```bash
sudo apt update && sudo apt install ruby cmake build-essential
```

## Running Tests

Run all tests:

```bash
./test/run_tests.sh
```

Or manually:

```bash
cmake -B build-test -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
make -C build-test -j$(nproc)
ctest --test-dir build-test --output-on-failure
```

## Code Coverage

Install lcov:

```bash
sudo apt install lcov
```

Generate coverage report:

```bash
./generate_coverage.sh
```

Or manually:

```bash
cmake -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
make -C build -j$(nproc)
ctest --test-dir build
lcov --capture --directory build --output-file build/coverage.info
lcov --remove build/coverage.info '*/test/*' '*/build/_deps/*' --output-file build/coverage_filtered.info
genhtml build/coverage_filtered.info --output-directory build/coverage_html
```

The HTML report at the location: `build/coverage_html/index.html`

## Adding New Tests

### Unit Tests

1. Create test file in `test/unit/test_<module>.c`:

```c
#include "unity.h"
#include "Mock<dependency>.h"  // Auto-generated mocks
#include "<module>.h"           // Module under test

void setUp(void) { /* Setup before each test */ }
void tearDown(void) { /* Cleanup after each test */ }

void test_<feature>_success(void) {
    // Arrange: Set up mocks
    Mock_function_ExpectAndReturn(arg, return_value);

    // Act: Call function
    int result = function_under_test(arg);

    // Assert: Verify result
    TEST_ASSERT_EQUAL(expected, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_<feature>_success);
    return UNITY_END();
}
```

2. Add to `test/unit/CMakeLists.txt`:

```cmake
add_unit_test(
    NAME test_<module>
    SOURCES test_<module>.c
    MOCKS <dependency>.h
)
```

### Integration Tests

1. Create test file in `test/integration/test_integration_<feature>.c`:

```c
#include "unity.h"
#include "<module>.h"  // Real implementation

void setUp(void) {}
void tearDown(void) {}

void test_<feature>_end_to_end(void) {
    // Test with real dependencies
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_<feature>_end_to_end);
    return UNITY_END();
}
```

2. Add to `test/integration/CMakeLists.txt`:

```cmake
add_integration_test(
    NAME test_integration_<feature>
    SOURCES test_integration_<feature>.c
)
```
