#!/bin/bash
# Generate missing graph.json files for runtime tests

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEST_DIR="$PROJECT_ROOT/test/integration/test_cases/runtime"

# Runtime test cases
RUNTIME_TESTS=("bull_patterns" "gap_analysis" "rsi_report")

echo "Generating graph.json files for runtime tests..."
echo "================================================"

for test in "${RUNTIME_TESTS[@]}"; do
    TEST_PATH="$TEST_DIR/$test"
    INPUT_FILE="$TEST_PATH/input.txt"
    OUTPUT_FILE="$TEST_PATH/expected/graph.json"

    echo ""
    echo "Processing: $test"

    if [ ! -f "$INPUT_FILE" ]; then
        echo "  ERROR: Input file not found: $INPUT_FILE"
        continue
    fi

    # Create expected directory if it doesn't exist
    mkdir -p "$TEST_PATH/expected"

    # Generate graph.json using Python helper script
    python3 "$SCRIPT_DIR/compile_to_json.py" "$INPUT_FILE" "$OUTPUT_FILE"

    if [ -f "$OUTPUT_FILE" ]; then
        echo "  ✓ Generated: $OUTPUT_FILE"
        # Show first few lines
        echo "  Preview:"
        head -n 3 "$OUTPUT_FILE" | sed 's/^/    /'
    else
        echo "  ERROR: Failed to generate $OUTPUT_FILE"
    fi
done

echo ""
echo "================================================"
echo "Done!"
