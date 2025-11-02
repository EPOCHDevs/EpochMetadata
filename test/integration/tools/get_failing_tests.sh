#!/bin/bash
# Extract failing test case names from integration test run

cd "$(dirname "$0")/../../../cmake-build-debug/bin" || exit 1

# Run integration tests and extract failures
./epoch_script_test "[epoch_script][integration]" -r console 2>&1 | \
    grep -E "^  [a-z_/]+" | \
    grep -v "passed" | \
    sed 's/^  //' | \
    sort -u

echo "Total failing tests: $(./epoch_script_test '[epoch_script][integration]' -r console 2>&1 | grep 'test cases:' | awk '{print $7}')" >&2
