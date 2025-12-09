#!/bin/bash
# Verify test setup

echo "Checking test infrastructure..."

# Check for required commands
for cmd in cmake make ruby gcc; do
    if ! command -v $cmd &> /dev/null; then
        echo "ERROR: $cmd not found"
        exit 1
    fi
done

echo "✓ Required commands found"

# Check for test files
if [ ! -f "test/unit/test_secure_tunnel.c" ]; then
    echo "ERROR: test_secure_tunnel.c not found"
    exit 1
fi

if [ ! -f "test/integration/test_integration_error_handling.c" ]; then
    echo "ERROR: test_integration_error_handling.c not found"
    exit 1
fi

echo "✓ Test files present"

# Check for test infrastructure
if [ ! -f "test/CMakeLists.txt" ]; then
    echo "ERROR: test/CMakeLists.txt not found"
    exit 1
fi

if [ ! -f "test/fc_deps.json" ]; then
    echo "ERROR: test/fc_deps.json not found"
    exit 1
fi

echo "✓ Test infrastructure present"

echo ""
echo "Test setup verified successfully!"
echo "Run './test/run_tests.sh' to build and run tests"
