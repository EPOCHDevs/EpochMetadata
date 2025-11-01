# EpochFlow Source Test Framework

Comprehensive testing framework for EpochFlow Python source code that validates DataFrames, TearSheets, and Selector data.

## Overview

This framework allows you to test EpochFlow DSL code end-to-end by:
1. Loading input market data from CSV files
2. Compiling and executing your EpochFlow source code
3. Comparing outputs against expected results (DataFrames, TearSheets, Selectors)

## Test Case Structure

Each test case is a directory containing:

```
test_case_name/
├── source.py                    # EpochFlow source code (required)
├── config.yaml                  # Test configuration (optional)
├── input/                       # Input data directory (required)
│   └── {timeframe}_{asset}.csv  # e.g., 1D_AAPL-Stocks.csv
└── expected/                    # Expected outputs (auto-generated)
    ├── dataframe/
    │   └── {timeframe}_{asset}.csv
    ├── selector/
    │   └── {asset}.json         # Asset-based only (no timeframe)
    └── tearsheet/
        └── {asset}.json         # Asset-based only (no timeframe)
```

### File Naming Conventions

**Input CSV files**: `{timeframe}_{symbol-assetclass}.csv`
- Examples:
  - `1D_AAPL-Stocks.csv`
  - `1Min_^EURUSD-FX.csv`
  - `15Min_ESH25-Futures.csv`

**Output DataFrame CSV files**: Same as input (timeframe + asset)
- `1D_AAPL-Stocks.csv`

**Output TearSheet/Selector JSON files**: Asset-based only (no timeframe)
- `AAPL-Stocks.json`
- `^EURUSD-FX.json`

### CSV Format

Input and output CSV files must have a `timestamp` column as the index:

```csv
timestamp,o,h,l,c,v,rsi#result,sma_50#result
2020-01-01T00:00:00,100.0,102.0,99.0,101.0,1000000,45.2,100.5
2020-01-02T00:00:00,101.0,103.0,100.5,102.5,1200000,52.3,101.2
```

- **Timestamp format**: ISO8601 (`YYYY-MM-DDTHH:MM:SS`)
- **Columns**: Any OHLCV + transform output columns
- **Missing values**: Leave empty or use standard CSV empty value representation

## Creating a New Test Case

### Method 1: Auto-Generate (Recommended)

1. **Create test directory structure**:
   ```bash
   mkdir -p test_name/input
   ```

2. **Write your EpochFlow source**:
   ```python
   # test_name/source.py
   src = market_data_source()

   rsi_val = rsi(period=14)(src.c)

   numeric_cards_report(
       title="RSI Value",
       category="Technical",
       agg="last"
   )(rsi_val)
   ```

3. **Add input data**:
   - Create CSV files in `input/` directory
   - Example: `input/1D_AAPL-Stocks.csv`

4. **Auto-generate expected outputs**:
   ```bash
   cd cpp/build
   GENERATE_EXPECTED=1 ./bin/epoch_stratifyx_flow_source_test "test_name"
   ```

5. **Review and approve**:
   - Check generated files in `expected/` directory
   - Verify DataFrames, TearSheets, Selectors are correct
   - Edit `config.yaml` and change `status: APPROVED`

6. **Run test normally**:
   ```bash
   ./bin/epoch_stratifyx_flow_source_test "test_name"
   ```

### Method 2: Manual Creation

1. Create test directory and `source.py`
2. Add input CSV files
3. Manually create expected output files:
   - DataFrames as CSV
   - TearSheets as JSON (protobuf JSON format)
   - Selectors as JSON (array format)
4. Create `config.yaml` with `status: APPROVED`

## Configuration File (config.yaml)

Optional configuration to override test behavior:

```yaml
title: "RSI(2) Report Test"
status: APPROVED              # or PENDING_REVIEW
assets: ["AAPL-Stocks"]       # Optional: override auto-detected assets
timeframes: ["1D"]            # Optional: override base timeframe
strict: false                 # Optional: strict timeframe inference
```

### Status Values

- **`APPROVED`**: Test will run and validate outputs
- **`PENDING_REVIEW`**: Test will be skipped (until manually reviewed and approved)

## Running Tests

### Run all FlowSource tests:
```bash
cd cpp/build
./bin/epoch_stratifyx_transform_test "[flow_source]"
```

### Run all transform tests (includes FlowSource):
```bash
./bin/epoch_stratifyx_transform_test
```

### Run specific test:
```bash
./bin/epoch_stratifyx_transform_test "rsi_report"
```

### Auto-generate expected outputs:
```bash
GENERATE_EXPECTED=1 ./bin/epoch_stratifyx_transform_test "[flow_source]"
```

### Run with verbose output:
```bash
./bin/epoch_stratifyx_transform_test "[flow_source]" -s  # Show successful tests
```

## Asset Auto-Detection

The framework automatically detects which assets are required:

### Single-Asset Tests
If source code doesn't use cross-sectional transforms, uses the first asset found in `input/` directory.

### Cross-Sectional Tests
If source code contains cross-sectional transforms (`cs_*`, `top_k`, `bottom_k`, `portfolio_*`), automatically loads **all** assets from `input/` directory.

Example cross-sectional source:
```python
src = market_data_source()
momentum = (src.c / src.c.shift(20) - 1)

# This triggers multi-asset mode
top_stocks = top_k(k=5)(momentum)
```

### Override Asset Detection
Use `config.yaml` to explicitly specify assets:
```yaml
assets:
  - "AAPL-Stocks"
  - "MSFT-Stocks"
  - "GOOGL-Stocks"
```

## Output Format Specifications

### DataFrame CSV
Standard CSV with timestamp index and column headers.

### TearSheet JSON
Protobuf JSON representation (auto-generated by framework):
```json
{
  "cards": {
    "cards": [
      {"title": "Win Rate", "value": 65.5, "format": "percentage"}
    ]
  },
  "charts": {
    "charts": [
      {"title": "Equity Curve", "type": "line", "series": [...]}
    ]
  },
  "tables": {
    "tables": [
      {"title": "Round Trips", "rows": [...]}
    ]
  }
}
```

### Selector JSON
Array of selector objects:
```json
[
  {
    "title": "Gap Analysis",
    "icon": "TrendingUp",
    "schemas": [
      {"columnName": "gap_size", "slot": "Hero", "renderType": "Number"},
      {"columnName": "fill_time", "slot": "PrimaryA", "renderType": "Duration"}
    ],
    "pivot_index": null,
    "data": {
      "columns": ["gap_size", "fill_time", "gap_filled"],
      "rows": [
        [0.025, 3600, true],
        [0.018, 5400, true]
      ]
    }
  }
]
```

## Example Test Cases

### Example 1: Simple RSI Report
```
rsi_report/
├── source.py
├── input/
│   └── 1D_AAPL-Stocks.csv
└── expected/
    ├── dataframe/
    │   └── 1D_AAPL-Stocks.csv
    └── tearsheet/
        └── AAPL-Stocks.json
```

**source.py**:
```python
src = market_data_source()

rsi_val = rsi(period=2)(src.c)

numeric_cards_report(
    title="RSI(2)",
    category="Indicators",
    agg="last"
)(rsi_val)
```

### Example 2: Gap Analysis with Selector
```
gap_analysis/
├── source.py
├── input/
│   └── 1Min_^EURUSD-FX.csv
└── expected/
    ├── dataframe/
    │   └── 1Min_^EURUSD-FX.csv
    ├── selector/
    │   └── ^EURUSD-FX.json
    └── tearsheet/
        └── ^EURUSD-FX.json
```

**source.py**:
```python
gaps = gap_classify(fill_percent=100, timeframe="1Min", session="London Session")()

gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)
```

### Example 3: Cross-Sectional Momentum
```
cross_sectional_momentum/
├── source.py
├── config.yaml
├── input/
│   ├── 1D_AAPL-Stocks.csv
│   ├── 1D_MSFT-Stocks.csv
│   └── 1D_GOOGL-Stocks.csv
└── expected/
    ├── dataframe/
    │   ├── 1D_AAPL-Stocks.csv
    │   ├── 1D_MSFT-Stocks.csv
    │   └── 1D_GOOGL-Stocks.csv
    └── tearsheet/
        ├── AAPL-Stocks.json
        ├── MSFT-Stocks.json
        └── GOOGL-Stocks.json
```

**source.py**:
```python
src = market_data_source()

momentum = (src.c / src.c.shift(20) - 1)
cs_rank = cs_rank()(momentum)
top_3 = top_k(k=3)(cs_rank)

numeric_cards_report(
    title="Momentum Rank",
    category="Cross-Sectional",
    agg="last"
)(cs_rank)
```

**config.yaml**:
```yaml
title: "Cross-Sectional Momentum Test"
status: APPROVED
assets:
  - "AAPL-Stocks"
  - "MSFT-Stocks"
  - "GOOGL-Stocks"
timeframes: ["1D"]
```

## Troubleshooting

### Test fails with "Expected file not found"
- Run with `GENERATE_EXPECTED=1` to create expected files
- Review generated files and approve in config.yaml

### DataFrame comparison fails
- Check column names match exactly
- Verify timestamp format is consistent
- Check for NaN handling differences

### TearSheet/Selector comparison fails
- Compare JSON diff output line-by-line
- Check for floating-point precision issues
- Verify enum string representations match

### Asset auto-detection incorrect
- Add explicit `assets` list in config.yaml
- Check that input CSV filenames follow naming convention

## Best Practices

1. **Keep tests focused**: One feature/transform per test case
2. **Use realistic data**: Base input CSVs on actual market data
3. **Auto-generate first**: Let framework create expected outputs, then review
4. **Commit expected files**: Check in all `expected/` files to git
5. **Test edge cases**: Include NaN, extreme values, empty datasets
6. **Document assumptions**: Use config.yaml title field to describe test

## Integration with CI/CD

Add to your build pipeline:
```bash
# Build tests
cmake --build build --target epoch_stratifyx_transform_test

# Run FlowSource tests
cd build
./bin/epoch_stratifyx_transform_test "[flow_source]"

# Exit code 0 = all tests passed
```

## See Also

- **EpochFlow DSL Reference**: `docs/EPOCHFLOW_DSL_REFERENCE.md`
- **YAML-based tests**: `flow_graph_test_cases/` (for testing transform configurations)
- **Transform documentation**: `docs/DataFlowRuntimeOrchestrator_Technical_Guide.md`
