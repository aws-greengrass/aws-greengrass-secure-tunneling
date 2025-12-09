#!/bin/bash
# Run tests for aws-greengrass-secure-tunnel

set -e

BUILD_DIR="${BUILD_DIR:-build-test}"

echo "Building tests..."
cmake -B "$BUILD_DIR" -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
make -C "$BUILD_DIR" -j$(nproc)

echo "Running tests..."
cd "$BUILD_DIR"
ctest --output-on-failure

echo "Tests completed successfully!"
