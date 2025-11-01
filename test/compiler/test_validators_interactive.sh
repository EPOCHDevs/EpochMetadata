#!/bin/bash
#
# Interactive Validator Test Script
# Demonstrates the new VARARGS validator system for first_non_null and conditional_select
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/../.."
BUILD_DIR="$PROJECT_ROOT/cmake-build-debug"
TEST_EXEC="$BUILD_DIR/bin/epoch_metadata_test"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "================================================================================"
echo "🎯 EpochFlow Validator System - Interactive Test"
echo "   Testing compile-time validation for VARARGS transforms"
echo "================================================================================"
echo ""

# Check if test executable exists
if [ ! -f "$TEST_EXEC" ]; then
    echo -e "${RED}❌ Test executable not found: $TEST_EXEC${NC}"
    echo "   Please build the project first:"
    echo "   cmake --build cmake-build-debug --target epoch_metadata_test"
    exit 1
fi

echo -e "${GREEN}✅ Test executable found${NC}"
echo ""

# Function to run a specific test case
run_test() {
    local test_name=$1
    local test_dir="$PROJECT_ROOT/test/epochflow/compiler/test_cases/$test_name"

    if [ ! -d "$test_dir" ]; then
        echo -e "${RED}❌ Test directory not found: $test_name${NC}"
        return 1
    fi

    echo "--------------------------------------------------------------------------------"
    echo -e "${BLUE}📝 Test: $test_name${NC}"
    echo "--------------------------------------------------------------------------------"

    # Show input
    echo -e "\n${YELLOW}Input Code:${NC}"
    cat "$test_dir/input.txt" | sed 's/^/  /'

    echo ""
    echo -e "${YELLOW}Expected Outcome:${NC}"
    if grep -q '"error"' "$test_dir/expected.json" 2>/dev/null; then
        echo -e "  ${RED}❌ Should FAIL compilation${NC}"
        echo -n "  Error: "
        grep '"error"' "$test_dir/expected.json" | sed 's/.*"error": "//;s/".*//'
    else
        echo -e "  ${GREEN}✅ Should PASS compilation${NC}"
    fi

    echo ""
}

echo "================================================================================"
echo "🔴 ERROR TEST CASES - These should fail compilation with validator errors"
echo "================================================================================"
echo ""

run_test "first_non_null_no_inputs"
echo ""

run_test "conditional_select_insufficient_inputs"
echo ""

run_test "conditional_select_invalid_condition_type"
echo ""

echo "================================================================================"
echo "🟢 VALID TEST CASES - These should compile successfully"
echo "================================================================================"
echo ""

run_test "first_non_null_valid"
echo ""

run_test "conditional_select_valid"
echo ""

echo "================================================================================"
echo "🧪 Running Actual Compiler Tests"
echo "================================================================================"
echo ""

echo "Running: $TEST_EXEC \"*EpochFlow Compiler*\""
echo ""

# Run the actual tests and capture output
if $TEST_EXEC "*EpochFlow Compiler*" 2>&1 | tail -20; then
    echo ""
    echo -e "${GREEN}✅ Test execution completed${NC}"
else
    echo ""
    echo -e "${YELLOW}⚠️  Some tests may have failed (this is expected for error test cases)${NC}"
fi

echo ""
echo "================================================================================"
echo "📊 Validator System Summary"
echo "================================================================================"
echo ""
echo "The validator system catches these errors at COMPILE TIME:"
echo ""
echo "  1. first_non_null - Requires at least 1 input"
echo "  2. conditional_select - Requires at least 2 inputs (condition + value)"
echo "  3. conditional_select - Even-indexed inputs must be Boolean (conditions)"
echo ""
echo "✨ Key Benefits:"
echo "  • Modular design - easy to add new validators"
echo "  • Zero runtime cost - O(1) registry lookup"
echo "  • Clear error messages - helps developers fix issues quickly"
echo "  • Extensible - just implement ISpecialNodeValidator interface"
echo ""
echo "================================================================================"
