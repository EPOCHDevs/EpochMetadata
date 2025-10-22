# EpochFlow AST Compiler Benchmark Module

Performance benchmarking and regression tracking for the EpochFlow AST compiler.

## Overview

This benchmark module establishes baseline performance metrics for the EpochFlow AST compiler compilation process, enabling:
- **Performance tracking** across different script complexities
- **Regression detection** when code changes impact compilation speed
- **Optimization validation** to measure improvements
- **CI/CD integration** for automated performance testing

## Quick Start

### Building Benchmarks

```bash
cd CmakeBuildDebug

# Build the benchmark executable
cmake --build . --target ast_compiler_benchmark

# Or rebuild entire project with benchmarks
cmake .. -DBUILD_TEST=ON
cmake --build .
```

### Running Benchmarks

```bash
# Quick run (baseline scenarios only, 10 samples)
make run_compiler_benchmarks_quick

# Full run (all scenarios, 100 samples)
make run_compiler_benchmarks

# Summary with baseline comparison
make compiler_benchmark_summary

# Update baseline (save new performance baseline)
make update_compiler_baseline
```

### Direct Execution

```bash
# Run all benchmarks with 100 samples
./bin/ast_compiler_benchmark --benchmark-samples 100

# Run only baseline scenarios
./bin/ast_compiler_benchmark "[baseline]" --benchmark-samples 100

# Exclude stress tests
./bin/ast_compiler_benchmark "~[stress]" --benchmark-samples 100

# Run summary with baseline update
UPDATE_BASELINE=1 ./bin/ast_compiler_benchmark "[summary]" --benchmark-samples 100
```

## Benchmark Scenarios

### Worst Case: Simple Script (Fastest)
**File:** `scripts/worst_case_simple.txt`
**Lines:** 3
**Description:** Minimal script with only literal values
**Expected Performance:** < 100 µs

```python
x = 5.0
y = True
z = "test"
```

**What it measures:**
- Minimal parsing overhead
- Basic AST node creation
- Literal materialization
- Baseline compiler startup cost

---

### Middle Case: Basic Strategy (Typical)
**File:** `scripts/middle_case_basic.txt`
**Lines:** 8
**Description:** Simple EMA crossover strategy
**Expected Performance:** < 500 µs

```python
src = market_data_source(timeframe="1H")
ema20 = ema(period=20)(src.c)
ema50 = ema(period=50)(src.c)
cross = crossover()(ema20.result, ema50.result)
trade_signal_executor()(enter_long=cross.result)
```

**What it measures:**
- Constructor parsing and validation
- Input wiring and handle resolution
- Timeframe propagation
- Component metadata lookup
- Typical user script compilation

---

### Best Case: Complex Strategy (Slowest)
**File:** `scripts/best_case_complex.txt`
**Lines:** 34
**Description:** Advanced consolidation box fade strategy
**Expected Performance:** < 2 ms

```python
# BTC Consolidation Box Fade Strategy
src = market_data_source(timeframe="15Min")

# Define parameters
lookback = 20
threshold = 0.005
breakout_pct = 0.5

# Box boundaries
upper_resistance = max(period=lookback)(src.h)
lower_support = min(period=lookback)(src.l)
box_range = upper_resistance - lower_support

# Volatility filter
price_stddev = stddev(period=lookback)(src.c)
avg_price = sma(period=lookback)(src.c)
vol_ratio = price_stddev / avg_price
is_consolidating = vol_ratio < threshold

# Entry signals
long_entry = (src.c <= lower_support) and is_consolidating
short_entry = (src.c >= upper_resistance) and is_consolidating

# Exit signals
long_exit = src.c > (upper_resistance + (box_range * breakout_pct))
short_exit = src.c < (lower_support - (box_range * breakout_pct))

# Trade execution
trade_signal_executor()(
    enter_long=long_entry,
    enter_short=short_entry,
    exit_long=long_exit,
    exit_short=short_exit
)
```

**What it measures:**
- Complex expression evaluation
- Binary and logical operators
- Variable resolution
- Multiple indicator chaining
- Large node graph creation
- Maximum compilation overhead

---

## Benchmark Tags

Filter benchmarks using tags:

| Tag | Description | Usage |
|-----|-------------|-------|
| `[compiler]` | All compiler benchmarks | `./bin/ast_compiler_benchmark "[compiler]"` |
| `[simple]` | Worst case (simplest) | `./bin/ast_compiler_benchmark "[simple]"` |
| `[medium]` | Middle case (typical) | `./bin/ast_compiler_benchmark "[medium]"` |
| `[complex]` | Best case (complex) | `./bin/ast_compiler_benchmark "[complex]"` |
| `[baseline]` | Core baseline scenarios | `./bin/ast_compiler_benchmark "[baseline]"` |
| `[critical]` | Critical path benchmarks | `./bin/ast_compiler_benchmark "[critical]"` |
| `[stress]` | Stress tests | `./bin/ast_compiler_benchmark "[stress]"` |
| `[edge]` | Edge cases | `./bin/ast_compiler_benchmark "[edge]"` |
| `[summary]` | Summary report | `./bin/ast_compiler_benchmark "[summary]"` |

### Combining Tags

```bash
# Run all baseline scenarios excluding stress tests
./bin/ast_compiler_benchmark "[baseline]~[stress]"

# Run only complex and critical scenarios
./bin/ast_compiler_benchmark "[complex],[critical]"
```

## Performance Metrics

### What is Measured

Each benchmark reports:
- **Mean time** - Average compilation time (primary metric)
- **Median time** - 50th percentile (robust to outliers)
- **Std deviation** - Consistency of measurements
- **Min/Max time** - Range of measurements
- **95% confidence interval** - Statistical confidence bounds
- **Number of samples** - Iterations performed

### Sample Output

```
benchmark name                       samples    iterations    estimated
                                    mean        low mean      high mean
                                    std dev     low std dev   high std dev

Compile simple script (3 lines)      100        1             12.5ms
                                    75.3µs      72.1µs        78.9µs
                                    8.2µs       6.5µs         10.3µs

Compile basic strategy (8 lines)     100        1             42.8ms
                                    428.6µs     415.2µs       443.7µs
                                    35.1µs      28.9µs        42.5µs

Compile complex strategy (34 lines)  100        1             153.2ms
                                    1.53ms      1.48ms        1.59ms
                                    142µs       119µs         168µs
```

## Baseline Management

### Creating Initial Baseline

```bash
# First time setup - establish baseline
cd CmakeBuildDebug
UPDATE_BASELINE=1 make compiler_benchmark_summary
```

This creates `baselines/compiler/ast_compiler_baseline.json`:

```json
{
  "version": "1.0",
  "updated": 1704931200,
  "benchmarks": [
    {
      "name": "Simple (Worst)",
      "mean_ms": 0.0753,
      "median_ms": 0.0721,
      "std_dev_ms": 0.0082,
      "min_ms": 0.0651,
      "max_ms": 0.0891,
      "samples": 100,
      "timestamp": "2025-10-19T00:00:00Z",
      "metadata": {
        "script_file": "worst_case_simple.txt",
        "script_size": "28"
      }
    }
  ]
}
```

### Comparing Against Baseline

```bash
# Run benchmarks and compare to baseline
make compiler_benchmark_summary
```

**Output:**
```
==========================================
AST Compiler Performance Summary
==========================================

Simple               :      75.30 µs | Baseline:      75.30 µs | Change:   +0.0%
Basic                :     428.60 µs | Baseline:     428.60 µs | Change:   +0.0%
Complex              :    1532.00 µs | Baseline:    1532.00 µs | Change:   +0.0%

==========================================
```

### Regression Detection

The benchmark automatically detects performance changes:

**Regression (>10% slower):**
```
REGRESSION DETECTED for 'Basic (Middle)':
  Baseline: 428.60 µs
  Current:  512.30 µs
  Regression: +19.5% (threshold: 10.0%)
```

**Improvement (>10% faster):**
```
IMPROVEMENT DETECTED for 'Complex (Best)':
  Baseline: 1532.00 µs
  Current:  1225.60 µs
  Improvement: 20.0%
```

**Stable (±10%):**
```
Performance stable for 'Simple (Worst)' (change: +3.2%)
```

### Updating Baseline

After confirming performance improvements or accepting new baseline:

```bash
UPDATE_BASELINE=1 make compiler_benchmark_summary
```

**⚠️ Important:** Only update baselines after:
1. Reviewing performance changes
2. Understanding why performance changed
3. Confirming changes are acceptable
4. Running multiple times for consistency

## CI/CD Integration

### GitHub Actions Example

```yaml
- name: Run Compiler Benchmarks
  run: |
    cd CmakeBuildDebug
    make run_compiler_benchmarks_ci

- name: Check for Regressions
  run: |
    python scripts/check_benchmark_regression.py \
      --baseline benchmark/baselines/compiler/ast_compiler_baseline.json \
      --current CmakeBuildDebug/ast_compiler_benchmark_results.json \
      --threshold 10
```

### JSON Output

Use `--reporter JSON::out=file.json` for CI/CD:

```bash
./bin/ast_compiler_benchmark "[baseline]" \
    --benchmark-samples 30 \
    --reporter JSON::out=results.json
```

## Best Practices

### DO ✅

- ✅ Run benchmarks on consistent hardware
- ✅ Close other applications during benchmarking
- ✅ Use sufficient sample size (100+ for baselines)
- ✅ Run multiple times before updating baseline
- ✅ Document significant performance changes
- ✅ Use Release or RelWithDebInfo build type
- ✅ Compare similar scenarios (simple vs simple)

### DON'T ❌

- ❌ Benchmark Debug builds (use Release/RelWithDebInfo)
- ❌ Run on laptop with thermal throttling
- ❌ Update baselines without review
- ❌ Ignore high standard deviation (>10% of mean)
- ❌ Mix different optimization levels
- ❌ Run benchmarks during heavy system load

## Troubleshooting

### High Standard Deviation

**Problem:** Std dev > 10% of mean indicates inconsistent results

**Solutions:**
- Close background applications
- Increase sample count: `--benchmark-samples 200`
- Check for thermal throttling
- Run on isolated hardware (CI server)

### Benchmark Hangs

**Problem:** Benchmark executable doesn't complete

**Solutions:**
- Check script files exist in `benchmark/scripts/`
- Verify transform registry initialization
- Run with `--list-tests` to see available benchmarks
- Check for infinite loops in test scripts

### Build Errors

**Problem:** Benchmark fails to compile

**Solutions:**
- Ensure `BUILD_TEST=ON` in CMake configuration
- Verify Catch2 is installed: `find_package(Catch2 3)`
- Check that `epoch_metadata` library builds successfully
- Verify BENCHMARK_SCRIPTS_DIR and BENCHMARK_BASELINES_DIR paths

### False Regressions

**Problem:** Benchmarks report regression but code hasn't changed

**Solutions:**
- Re-run with more samples
- Check system load (htop/top)
- Review standard deviation in results
- Run at different times of day
- Update baseline if system changed

## Directory Structure

```
benchmark/
├── CMakeLists.txt                          # Build configuration
├── README.md                                # This file
├── compiler/
│   └── ast_compiler_benchmark.cpp          # AST compiler benchmarks
├── common/
│   ├── benchmark_utils.h                   # Utilities (load/save/compare)
│   └── catch_benchmark_main.cpp            # Custom main with initialization
├── baselines/
│   └── compiler/
│       └── ast_compiler_baseline.json      # Performance baselines
└── scripts/
    ├── worst_case_simple.txt               # 3 lines - literals
    ├── middle_case_basic.txt               # 8 lines - basic strategy
    └── best_case_complex.txt               # 34 lines - complex strategy
```

## Performance Goals

### Current Baseline (to be established)

| Scenario | Target | Actual | Status |
|----------|--------|--------|--------|
| Worst (Simple) | < 100 µs | TBD | ⏳ Pending |
| Middle (Basic) | < 500 µs | TBD | ⏳ Pending |
| Best (Complex) | < 2 ms | TBD | ⏳ Pending |

### Future Optimization Targets

| Optimization | Improvement | Scenario |
|--------------|-------------|----------|
| Symbol table caching | 10-20% | All |
| AST node pooling | 15-25% | Complex |
| Component metadata cache | 5-10% | All |
| Lazy timeframe resolution | 10-15% | Medium/Complex |

## Additional Resources

- **Catch2 Documentation:** https://github.com/catchorg/Catch2/blob/devel/docs/benchmarks.md
- **Google Benchmark Comparison:** https://github.com/google/benchmark
- **Performance Testing Best Practices:** https://easyperf.net/blog/

## Contributing

When adding new benchmark scenarios:

1. Add script to `scripts/` directory
2. Create benchmark test case with appropriate tags
3. Document expected performance in this README
4. Run initial baseline and save results
5. Add to summary report if applicable

## Support

For issues or questions:
- File an issue in the repository
- Check existing baselines for reference
- Review Catch2 benchmark documentation
- Consult with team about performance expectations
