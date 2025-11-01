# EpochFlow Script Language

**Declarative Language for Quantitative Trading Strategies and Market Research**

Version 2.0 | Last Updated: 2025-01-01

---

## Overview

EpochFlow is a Python-like domain-specific language designed for institutional traders to build sophisticated quantitative trading strategies and conduct market research. It provides access to 300+ technical indicators, statistical functions, and market microstructure tools through a simple, type-safe syntax.

### Key Capabilities

- **Technical Analysis**: 100+ indicators (moving averages, oscillators, volatility measures)
- **Market Microstructure**: Session analysis, gap detection, Smart Money Concepts (SMC)
- **Statistical Tools**: Correlation, regression, volatility estimators, z-scores
- **Multi-Timeframe**: Combine daily, hourly, and intraday data in one strategy
- **Cross-Sectional**: Factor models, momentum rankings, universe filtering
- **Fundamental Data**: Balance sheets, income statements, economic indicators
- **Pattern Recognition**: Chart formations, candlestick patterns, structural analysis

### Design Philosophy

EpochFlow is **declarative**, not imperative. You describe *what* you want, not *how* to compute it:

- No loops, no if statements, no function definitions
- Variables are immutable (single assignment)
- Transforms are composable building blocks
- Strong type system with automatic inference
- Execution is vectorized and optimized

## Quick Start Example

```python
# Simple MA crossover strategy
src = market_data_source()
fast = ema(period=20)(src.c)
slow = ema(period=50)(src.c)

# Entry signals
buy = crossover(fast, slow)
sell = crossover(slow, fast)

# Execute trades
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

## 5-Minute Tutorial

### 1. Access Market Data

```python
src = market_data_source()
close = src.c      # Close price
high = src.h       # High price
volume = src.v     # Volume
```

### 2. Apply Indicators

```python
# Single output
rsi_val = rsi(period=14)(close)

# Multiple outputs (tuple unpacking)
lower, middle, upper = bbands(period=20, stddev=2)(close)
```

### 3. Create Trading Logic

```python
# Boolean conditions
oversold = rsi_val < 30
breakout = close > upper

# Combine conditions
entry = oversold and breakout
```

### 4. Historical Data (Lag Operator)

```python
prev_close = close[1]      # Previous bar
week_ago = close[5]        # 5 bars ago
momentum = close - close[10]
```

### 5. Execute Strategy

```python
trade_signal_executor()(
    enter_long=buy_signal,
    enter_short=sell_signal,
    exit_long=exit_signal
)
```

## Common Use Cases

### Technical Trading
- Trend following with moving averages
- Mean reversion with RSI/Stochastic
- Breakout detection with Bollinger Bands
- Momentum strategies with MACD

### Quantitative Strategies
- Cross-sectional momentum (factor models)
- Statistical arbitrage (pairs trading)
- Volatility targeting
- Multi-factor alpha models

### Intraday Trading
- Session-based strategies (London, New York)
- Gap analysis (overnight gaps, intraday halts)
- Smart Money Concepts (order blocks, liquidity)
- Opening range breakouts

### Research & Analysis
- Correlation studies (lead-lag relationships)
- Pattern detection (head and shoulders, triangles)
- Gap fill behavior analysis
- Calendar effect research

## Documentation Structure

1. **[Language Fundamentals](01-language-fundamentals.md)** - Syntax, types, operators, critical limitations
2. **[Transform System](02-transform-system.md)** - How transforms work, timeframes, sessions, cross-sectional
3. **[Core Transforms](03-core-transforms.md)** - Essential indicators and functions (80+ curated transforms)
4. **[Strategy Patterns](04-strategy-patterns.md)** - Complete strategy examples by trading style
5. **[Research Workflows](05-research-workflows.md)** - Market research vs live trading strategies
6. **[Advanced Topics](06-advanced-topics.md)** - Multi-condition logic, optimization, best practices
7. **[Error Reference](07-error-reference.md)** - Troubleshooting guide with solutions

**Appendices:**
- **[Full Transform Catalog](appendix-full-catalog.md)** - Complete list of 300+ transforms
- **[Glossary](glossary.md)** - Technical terms and definitions

## Strategy vs Research

### Trading Strategy (requires executor)
```python
src = market_data_source()
signal = rsi(period=14)(src.c) < 30
trade_signal_executor()(enter_long=signal)
```

### Research Script (no executor)
```python
# Analyze gap fill behavior
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled, gaps.gap_retrace, gaps.gap_size,
    gaps.psc, gaps.psc_timestamp
)
```

Research scripts output reports (charts, tables, cards) instead of trading signals.

## Multi-Timeframe Example

```python
src = market_data_source()

# Daily trend filter
daily_trend = ema(period=50, timeframe="1D")(src.c)
uptrend = src.c > daily_trend

# Intraday entry (15-minute bars)
fast = ema(period=12)(src.c)
slow = ema(period=26)(src.c)
cross = crossover(fast, slow)

# Only enter with daily trend
buy = cross and uptrend
trade_signal_executor()(enter_long=buy)
```

## Session-Based Example

```python
src = market_data_source()

# Detect London session
london = sessions(session_type="London")()

# Calculate EMA only during London session
london_ema = ema(period=20, session="London")(src.c)

# Breakout from session range
breakout = src.c > london.high
entry = london.active and breakout

trade_signal_executor()(enter_long=entry)
```

## Cross-Sectional Example

```python
# Momentum factor strategy (requires universe of stocks)
returns = roc(period=20)(close)
momentum = cs_momentum()(returns)

# Select top 10 stocks
top10 = top_k(k=10)(momentum)

trade_signal_executor()(enter_long=top10)
```

## Key Limitations

EpochFlow is **not** full Python. These constructs are prohibited:

### ❌ No Control Flow
```python
# NOT ALLOWED
if price > 100:
    signal = True

# USE INSTEAD
signal = price > 100
# Or ternary: signal = True if price > 100 else False
```

### ❌ No Loops
```python
# NOT ALLOWED
for i in range(10):
    calculate()

# USE INSTEAD
# Use vectorized transforms (they operate on entire series)
```

### ❌ No Reassignment
```python
# NOT ALLOWED
price = src.c
price = src.o  # Error!

# USE INSTEAD
close_price = src.c
open_price = src.o
```

### ❌ No Chained Comparisons
```python
# NOT ALLOWED
valid = 30 < rsi_val < 70

# USE INSTEAD
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid = above_30 and below_70
```

### ❌ No Zero Lag
```python
# NOT ALLOWED
current = src.c[0]

# USE INSTEAD
current = src.c
```

## Critical Rules

1. **Single Assignment**: Each variable assigned exactly once
2. **No Control Flow**: Use ternary operators and conditional transforms
3. **Integer Lag Only**: `src.c[1]` ✓, `src.c[0]` ✗, `src.c[1.5]` ✗
4. **Multi-Output Unpacking**: Count must match (use `_` to discard)
5. **One Executor**: Trading strategies need exactly one `trade_signal_executor()`
6. **Valid Timeframes**: `"1H"`, `"15Min"`, `"1D"` (not `"1Hour"` or `"1Day"`)

## Getting Help

- **Syntax errors**: See [Error Reference](07-error-reference.md)
- **Transform documentation**: See [Core Transforms](03-core-transforms.md)
- **Strategy examples**: See [Strategy Patterns](04-strategy-patterns.md)
- **Full transform list**: See [Appendix](appendix-full-catalog.md)

## Next Steps

- **New users**: Start with [Language Fundamentals](01-language-fundamentals.md)
- **Experienced traders**: Jump to [Strategy Patterns](04-strategy-patterns.md)
- **Researchers**: See [Research Workflows](05-research-workflows.md)
- **Troubleshooting**: Check [Error Reference](07-error-reference.md)

---

**For institutional traders building systematic strategies**
