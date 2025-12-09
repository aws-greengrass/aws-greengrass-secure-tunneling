# Testing

## Prerequisites

- Ruby (for CMock generation)
- CMake >= 3.22
- GCC or Clang

## Running Tests

```bash
./test/run_tests.sh
```

Or manually:

```bash
cmake -B build-test -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
make -C build-test -j$(nproc)
cd build-test && ctest --output-on-failure
```

## Test Structure

- `unit/` - Unit tests with mocking
- `integration/` - Integration tests
- `test_helpers.c/h` - Common test utilities

## Coverage (Optional)

To generate coverage reports, install lcov:

```bash
sudo apt install lcov
```

Then build with coverage flags:

```bash
cmake -B build-test -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
make -C build-test -j$(nproc)
cd build-test && ctest
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```
