# Documentation Code Snippet Tests

This directory contains automated tests for code snippets from the EpochScript documentation.

## Overview

All code examples in the `/docs` directory are automatically extracted, cataloged, and tested to ensure they remain valid as the language evolves. This provides:

- **Documentation accuracy**: Ensures examples in docs actually work
- **Regression protection**: Detects when code changes break documented patterns
- **Living documentation**: Documentation is backed by executable tests

## Directory Structure

```
documentation/
├── concepts/          # Tests from docs/concepts/
├── errors/            # Tests from docs/design-guidelines.md (error examples)
├── language/          # Tests from docs/language/
├── patterns/          # Tests from docs/strategies/patterns/
│   ├── combinations/  # Multi-indicator patterns
│   ├── fundamental/   # Fundamental/macro patterns
│   ├── intraday/      # Intraday trading patterns
│   ├── quantitative/  # Quantitative strategies
│   └── technical/     # Technical analysis patterns
└── strategies/        # Tests from docs/strategies/
```

## Test Case Format

Each test case directory contains:

```
test_name/
├── input.txt          # EpochScript code with source reference header
└── expected/
    └── graph.json     # Expected compiler output
```

Example `input.txt`:
```epochscript
# Source: language/index.md:18
# Section: Language Fundamentals > Quick Start
# Type: complete_strategy

src = market_data_source()
fast = ema(period=20)(src.c)
slow = ema(period=50)(src.c)
buy = crossover(fast, slow)
sell = crossover(slow, fast)
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

## Automated Workflow

### 1. Extract Snippets from Documentation

```bash
python3 scripts/extract_doc_snippets.py
```

**Output**: `doc-snippet-catalog.json` - Complete catalog of all code snippets found in documentation

**Statistics**:
- Total snippets: 329 across 38 markdown files
- Complete strategies: 22
- Partial examples: 97
- Syntax demos: 182
- Error examples: 26

### 2. Generate Test Cases

Generate test cases for complete strategies:
```bash
python3 scripts/generate_doc_test_cases.py strategies
```

Generate all testable snippets:
```bash
python3 scripts/generate_doc_test_cases.py all
```

List testable snippets without generating:
```bash
python3 scripts/generate_doc_test_cases.py list
```

### 3. Verify and Report

Run verification to test all generated cases:
```bash
python3 scripts/verify_doc_coverage.py
```

**Output**:
- Console report showing pass/fail for each test
- `doc-coverage-report.json` - Detailed results in JSON format

## Current Test Coverage

**Status**: 8/9 tests passing (88.9%)

| Category               | Tests | Passed | Failed | Rate   |
|------------------------|-------|--------|--------|--------|
| concepts               | 1     | 1      | 0      | 100.0% |
| errors                 | 1     | 1      | 0      | 100.0% |
| language               | 1     | 1      | 0      | 100.0% |
| patterns/combinations  | 1     | 1      | 0      | 100.0% |
| patterns/fundamental   | 1     | 0      | 1      | 0.0%   |
| patterns/intraday      | 1     | 1      | 0      | 100.0% |
| patterns/quantitative  | 1     | 1      | 0      | 100.0% |
| patterns/technical     | 1     | 1      | 0      | 100.0% |
| strategies             | 1     | 1      | 0      | 100.0% |

### Known Issues

**fundamental-macro_earnings_momentum** - Uses invalid option 'indicator' for economic_indicator component
- **Fix**: Update documentation to use correct option name

## Snippet Classification

The extraction script classifies snippets by testability:

| Type              | Description                                    | Testable |
|-------------------|------------------------------------------------|----------|
| complete_strategy | Full strategy with source and executor        | ✅ Yes   |
| complete_example  | Complete but simple example                    | ✅ Yes   |
| partial_example   | Needs additional context/data                  | ⚠️ Maybe |
| syntax_demo       | Syntax demonstration only                      | ❌ No    |
| error_example     | Expected to fail (anti-patterns)               | ⚠️ Special|

## Integration with Test Suite

These tests are part of the main integration test suite:

```bash
# Build tests
cmake --build cmake-build-debug --target epoch_script_test

# Run all integration tests (note: test discovery currently has issues)
./cmake-build-debug/bin/epoch_script_test "[integration]"

# Verify specific test with generate_graph tool
./cmake-build-debug/bin/generate_graph \
    test/integration/test_cases/documentation/language/language_quick_start/input.txt
```

## Adding New Documentation Examples

When adding new code examples to documentation:

1. **Write the example** in the appropriate markdown file
2. **Tag the code block** with ```epochscript
3. **Run extraction**: `python3 scripts/extract_doc_snippets.py`
4. **Generate test case**: `python3 scripts/generate_doc_test_cases.py strategies`
5. **Verify it works**: `python3 scripts/verify_doc_coverage.py`
6. **Commit** both the documentation and test case

## Maintenance

### Regenerating Test Cases

If the compiler output format changes:

```bash
# Re-generate all expected outputs
rm -rf test/integration/test_cases/documentation/*/expected/
python3 scripts/generate_doc_test_cases.py strategies
```

### Updating Catalog

After documentation changes:

```bash
python3 scripts/extract_doc_snippets.py
python3 scripts/generate_doc_test_cases.py all
python3 scripts/verify_doc_coverage.py
```

## Future Enhancements

- [ ] Test partial examples with mock data
- [ ] Verify error examples fail with expected messages
- [ ] Add runtime tests for examples with input data
- [ ] Generate HTML coverage report with links to source docs
- [ ] Add badges to documentation showing test status
- [ ] CI/CD integration to auto-update on doc changes

## Scripts

| Script                          | Purpose                                |
|---------------------------------|----------------------------------------|
| `extract_doc_snippets.py`       | Extract code from markdown files       |
| `generate_doc_test_cases.py`    | Generate test cases from catalog       |
| `verify_doc_coverage.py`        | Test all cases and generate report     |

All scripts are in `/scripts/` directory and can be run from the project root.
