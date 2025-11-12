#!/bin/bash
#
# Add default runtime validation to migrated JSON test files
# Sets all columns to "at_least_one_valid" by default
#

set -e

JSON_DIR="${1:-test/integration/test_cases_json}"

echo "Adding default runtime validation to JSON files in: $JSON_DIR"
echo ""

count=0

# Find all JSON files with empty runtime objects
for json_file in $(find "$JSON_DIR" -name "*.json" -type f); do
    # Check if file has "runtime": {}
    if grep -q '"runtime":\s*{}' "$json_file" 2>/dev/null; then
        echo "Processing: $json_file"

        # For now, just add a placeholder structure
        # In a real implementation, you'd analyze the graph to determine output columns
        # This is a simple version that adds basic validation structure

        python3 << 'PYTHON_SCRIPT' "$json_file"
import json
import sys

json_file = sys.argv[1]

try:
    with open(json_file, 'r') as f:
        data = json.load(f)

    # Only process if runtime is an empty dict
    if 'runtime' in data and data['runtime'] == {}:
        # Add default validation - we'll set generic validation
        # In practice, you'd analyze the graph to find actual output columns
        data['runtime'] = {
            "executor_outputs": {
                "columns": {
                    "enter_long": "at_least_one_valid",
                    "exit_long": "at_least_one_valid",
                    "position": "at_least_one_valid"
                }
            },
            "tearsheets": {
                "columns": {
                    "total_return": "at_least_one_valid",
                    "sharpe_ratio": "at_least_one_valid"
                }
            }
        }

        # Write back
        with open(json_file, 'w') as f:
            json.dump(data, f, indent=3)

        print(f"  ✓ Added default validation")
        sys.exit(0)
    else:
        print(f"  - Skipped (runtime not empty)")
        sys.exit(1)

except Exception as e:
    print(f"  ✗ Error: {e}")
    sys.exit(1)
PYTHON_SCRIPT

        if [ $? -eq 0 ]; then
            ((count++))
        fi
    fi
done

echo ""
echo "Processed $count files with default runtime validation"
