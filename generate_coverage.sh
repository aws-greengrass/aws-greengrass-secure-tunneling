#!/bin/bash
set -e

BUILD_DIR="build-test"

# Clean previous build
rm -rf "$BUILD_DIR"

# Configure with coverage
cmake -B "$BUILD_DIR" -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

# Build
make -C "$BUILD_DIR" -j$(nproc)

# Run tests
ctest --test-dir "$BUILD_DIR" --output-on-failure

# Generate coverage data
lcov --capture --directory "$BUILD_DIR" --output-file "$BUILD_DIR/coverage.info"

# Filter out system and test files
lcov --remove "$BUILD_DIR/coverage.info" '*/test/*' '*/build-test/_deps/*' --output-file "$BUILD_DIR/coverage_filtered.info"

# Generate HTML report
genhtml "$BUILD_DIR/coverage_filtered.info" --output-directory "$BUILD_DIR/coverage_html"

echo "Coverage report generated at: $BUILD_DIR/coverage_html/index.html"
