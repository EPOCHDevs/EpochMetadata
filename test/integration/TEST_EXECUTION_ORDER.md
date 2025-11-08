# Integration Test Execution Order

## Critical Finding: Test Pollution in Random Order

### Problem

Running integration tests with **random order** (Catch2 default) produces **24 false failures** due to test pollution:

```bash
# Random order (default): 37 failures
cmake-build-debug/bin/epoch_script_test

# Lexicographic order: 13 failures (correct)
cmake-build-debug/bin/epoch_script_test --order lex
```

### Root Cause

**Test State Leakage**: Tests share global state in the C++ runtime:
- Transform metadata cache
- Timeframe registry
- Singleton objects

When tests run in random order, an earlier test corrupts state that breaks later tests.

### Symptoms

Tests that **only fail in random order** get this error:
```
Assertion failed: fallbackTimeframe
Message: Timeframe is required for number_0/text_0/...
```

### Affected Tests (24 total)

These tests PASS in lexicographic order but FAIL in random order:
- basic/simple_literal
- constants/* (12 tests)
- variables/* (3 tests)
- documentation/language/syntax_expressions
- literals/* (3 tests)
- And 3 more...

### Solution

**Always use `--order lex` flag** for deterministic, reproducible test results.

### Test Workflows

#### Recommended (Lexicographic Order)
```bash
# Use the test workflow script (automatically uses --order lex)
./scripts/run_tests_with_reports.sh

# Or manually with lexicographic order
cmake-build-debug/bin/epoch_script_test --order lex
```

#### Not Recommended (Random Order)
```bash
# This will produce 24 false failures due to test pollution
cmake-build-debug/bin/epoch_script_test  # DON'T USE
```

### Real Failures

The **13 real failures** that occur in both modes:
1. basic/basic_success - CSV validation (missing AAPL asset)
2. basic/simple_operator - CSV validation
3. constants/constant_inline_arithmetic - CSV validation
4. constants/constant_subscript_arithmetic - CSV validation
5. control_flow/conditional_select_valid - CSV validation
6. control_flow/ternary_select - CSV validation
7. graphs/deeply_nested_expressions - CSV validation
8. graphs/multi_sinks - CSV validation
9. graphs/multi_slot_connections - CSV validation
10. operators/assoc_ops - CSV validation
11. operators/binary_operators - CSV validation
12. runtime/bull_patterns - CSV validation
13. runtime/gap_analysis - Event marker issue

### Future Work

To fix the test pollution (requires production code changes):
1. Add proper test isolation/cleanup in C++ test harness
2. Reset global state between test cases
3. Use test fixtures that initialize clean state
4. Consider splitting integration tests into separate executables

### References

- Test comparison: `/tmp/test_default.log` vs `/tmp/test_xml.log`
- Error tracking: `test/integration/test_cases/*/error.txt`
- Test workflow: `scripts/run_tests_with_reports.sh`
