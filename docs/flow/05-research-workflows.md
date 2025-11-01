# Research Workflows

Market research and analysis using EpochFlow (non-trading scripts).

---

## Table of Contents

1. [Research vs Strategy](#research-vs-strategy)
2. [Gap Analysis](#gap-analysis)
3. [Correlation Studies](#correlation-studies)
4. [Pattern Research](#pattern-research)
5. [Factor Analysis](#factor-analysis)
6. [Calendar Effects](#calendar-effects)
7. [Research Dashboard Workflow](#research-dashboard-workflow)

---

## Research vs Strategy

### Key Differences

| Aspect | Trading Strategy | Research Script |
|--------|-----------------|-----------------|
| **Output** | Trade signals | Reports (charts, tables, cards) |
| **Executor** | Required (`trade_signal_executor`) | Not used |
| **Purpose** | Generate trades for backtesting/live | Analyze data, find patterns |
| **Look-ahead** | Prohibited (causes bias) | Allowed (historical analysis) |
| **Sink** | `trade_signal_executor()` | `gap_report()`, `table_report()`, etc. |

### Trading Strategy Example

```python
# MUST have trade_signal_executor
src = market_data_source()
signal = rsi(period=14)(src.c) < 30
trade_signal_executor()(enter_long=signal)  # Required
```

### Research Script Example

```python
# NO trade_signal_executor
# Outputs reports instead
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)
```

---

## Gap Analysis

### Overnight Gap Research

**Question:** Do overnight gaps fill? How long does it take?

```python
# Detect overnight gaps in 1-minute data
gaps = session_gap(fill_percent=100, timeframe="1Min")()

# Comprehensive gap report
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,      # Boolean: Did gap fill?
    gaps.gap_retrace,     # Percent: How much retraced?
    gaps.gap_size,        # Percent: Gap size
    gaps.psc,             # Price: Previous session close
    gaps.psc_timestamp    # Time: Previous close timestamp
)
```

**Report Output:**
- **Cards**: Fill rate %, average gap size, median time to fill
- **Histograms**: Gap size distribution, fill time distribution
- **Tables**: Gaps by hour, gaps by size bucket, gaps by day of week

**Research Questions:**
- What % of gaps fill within the first hour?
- Do larger gaps fill less frequently?
- Does fill rate vary by day of week?
- What's the distribution of gap sizes?

### Intraday Gap Research (Trading Halts)

**Question:** How do trading halt gaps behave?

```python
# Detect intraday bar-to-bar gaps (2% threshold for stocks)
intraday_gaps = bar_gap(fill_percent=100, min_gap_size=2.0, timeframe="1Min")()

gap_report(fill_time_pivot_hour=13, histogram_bins=10)(
    intraday_gaps.gap_filled,
    intraday_gaps.gap_retrace,
    intraday_gaps.gap_size,
    intraday_gaps.psc,
    intraday_gaps.psc_timestamp
)
```

**Use Cases:**
- Trading halt analysis
- News event impact
- Liquidity event detection
- Circuit breaker behavior

### FX Pip Gap Analysis

**Question:** Do small forex gaps (4 pips) fill?

```python
# Detect 4-pip gaps in FX (0.04%)
fx_gaps = bar_gap(fill_percent=100, min_gap_size=0.04, timeframe="1Min")()

gap_report(fill_time_pivot_hour=12, histogram_bins=8)(
    fx_gaps.gap_filled,
    fx_gaps.gap_retrace,
    fx_gaps.gap_size,
    fx_gaps.psc,
    fx_gaps.psc_timestamp
)
```

### Gap Behavior by Session

**Question:** Do gaps behave differently by market session?

```python
# Asian session gaps
asian_gaps = session_gap(
    fill_percent=100,
    timeframe="1Min",
    session="AsianKillZone"
)()

# London gaps
london_gaps = session_gap(
    fill_percent=100,
    timeframe="1Min",
    session="London"
)()

# NY gaps
ny_gaps = session_gap(
    fill_percent=100,
    timeframe="1Min",
    session="NewYork"
)()

# Generate separate reports for comparison
gap_report(fill_time_pivot_hour=12)(
    asian_gaps.gap_filled,
    asian_gaps.gap_retrace,
    asian_gaps.gap_size,
    asian_gaps.psc,
    asian_gaps.psc_timestamp
)
```

---

## Correlation Studies

### Lead-Lag Analysis

**Question:** Does crypto volatility predict tech stock drawdowns?

```python
# Data sources
crypto = market_data_source()  # BTC
tech = market_data_source()    # QQQ or tech stock

# Calculate metrics
crypto_vol = volatility(period=20)(crypto.c)
tech_dd = ulcer_index(period=20)(tech.c)

# Concurrent correlation
concurrent_corr = rolling_corr(window=20)(x=crypto_vol, y=tech_dd)

# Lagged correlations (test multiple lags)
crypto_vol_lag1 = crypto_vol[1]
crypto_vol_lag5 = crypto_vol[5]
crypto_vol_lag10 = crypto_vol[10]

lag1_corr = rolling_corr(window=20)(x=crypto_vol_lag1, y=tech_dd)
lag5_corr = rolling_corr(window=20)(x=crypto_vol_lag5, y=tech_dd)
lag10_corr = rolling_corr(window=20)(x=crypto_vol_lag10, y=tech_dd)

# Generate table comparing correlations
table_report(sql="""
    SELECT
        date,
        concurrent_corr,
        lag1_corr,
        lag5_corr,
        lag10_corr
    FROM input
    WHERE concurrent_corr IS NOT NULL
    ORDER BY date DESC
    LIMIT 100
""")(
    date=timestamp,
    concurrent_corr=concurrent_corr,
    lag1_corr=lag1_corr,
    lag5_corr=lag5_corr,
    lag10_corr=lag10_corr
)
```

**Analysis:**
- If `lag5_corr > concurrent_corr` consistently: crypto leads tech by 5 days
- If `concurrent_corr` highest: simultaneous relationship
- If lagged correlations all lower: no predictive power

### Correlation Stability Research

**Question:** How stable are asset correlations over time?

```python
asset1 = market_data_source()
asset2 = market_data_source()

# Short and long window correlations
corr_short = rolling_corr(window=20)(x=asset1.c, y=asset2.c)
corr_long = rolling_corr(window=60)(x=asset1.c, y=asset2.c)

# Correlation divergence
corr_divergence = abs(corr_short - corr_long)

# High divergence = regime shift
regime_shift = corr_divergence > 0.3

# Table of regime shifts
table_report(sql="""
    SELECT
        date,
        corr_short,
        corr_long,
        corr_divergence,
        regime_shift
    FROM input
    WHERE regime_shift = 1
    ORDER BY date DESC
""")(
    date=timestamp,
    corr_short=corr_short,
    corr_long=corr_long,
    corr_divergence=corr_divergence,
    regime_shift=regime_shift
)
```

### Cross-Asset Correlation Matrix

**Question:** Which assets are most correlated?

```python
# Multiple assets (requires multi-asset setup)
asset_a = market_data_source()
asset_b = market_data_source()
asset_c = market_data_source()

# Pairwise correlations
corr_ab = rolling_corr(window=60)(x=asset_a.c, y=asset_b.c)
corr_ac = rolling_corr(window=60)(x=asset_a.c, y=asset_c.c)
corr_bc = rolling_corr(window=60)(x=asset_b.c, y=asset_c.c)

# Average correlation (diversification measure)
avg_corr = (corr_ab + corr_ac + corr_bc) / 3.0

# Table of recent correlations
table_report(sql="""
    SELECT
        date,
        corr_ab,
        corr_ac,
        corr_bc,
        avg_corr
    FROM input
    ORDER BY date DESC
    LIMIT 50
""")(
    date=timestamp,
    corr_ab=corr_ab,
    corr_ac=corr_ac,
    corr_bc=corr_bc,
    avg_corr=avg_corr
)
```

---

## Pattern Research

### Head and Shoulders Frequency

**Question:** How often do H&S patterns occur? Success rate?

```python
src = market_data_source()

# Detect H&S patterns
hs = head_and_shoulders(tolerance=0.02)()

# Pattern characteristics
pattern_count = hs.pattern_detected  # Boolean (count TRUEs)
neckline_price = hs.neckline
target_price = hs.target

# Breakdown signal
breakdown = src.c < hs.neckline

# Target hit (price reaches projected target)
target_hit = src.c <= hs.target

# Table of patterns
table_report(sql="""
    SELECT
        date,
        pattern_detected,
        neckline_price,
        target_price,
        breakdown,
        target_hit,
        COUNT(*) as pattern_count
    FROM input
    WHERE pattern_detected = 1
    GROUP BY date, pattern_detected, neckline_price, target_price, breakdown, target_hit
    ORDER BY date DESC
""")(
    date=timestamp,
    pattern_detected=hs.pattern_detected,
    neckline_price=neckline_price,
    target_price=target_price,
    breakdown=breakdown,
    target_hit=target_hit
)
```

### Triangle Breakout Analysis

**Question:** Do triangle breakouts succeed? Directional bias?

```python
src = market_data_source()

# Detect triangles
tri = triangles(min_pattern_bars=15)()

# Pattern types
ascending = tri.ascending
descending = tri.descending
symmetrical = tri.symmetrical

# Breakout direction
breakout_up = tri.breakout and (src.c > tri.upper_line)
breakout_down = tri.breakout and (src.c < tri.lower_line)

# Measure follow-through (5 bars after breakout)
future_return_5 = (src.c[-5] - src.c) / src.c  # Look-ahead (research only)

# Table of breakout performance
table_report(sql="""
    SELECT
        CASE
            WHEN ascending = 1 THEN 'Ascending'
            WHEN descending = 1 THEN 'Descending'
            WHEN symmetrical = 1 THEN 'Symmetrical'
        END as pattern_type,
        CASE
            WHEN breakout_up = 1 THEN 'Up'
            WHEN breakout_down = 1 THEN 'Down'
        END as breakout_direction,
        AVG(future_return_5) as avg_return_5d,
        STDDEV(future_return_5) as vol_5d,
        COUNT(*) as sample_count
    FROM input
    WHERE breakout_up = 1 OR breakout_down = 1
    GROUP BY pattern_type, breakout_direction
""")(
    ascending=ascending,
    descending=descending,
    symmetrical=symmetrical,
    breakout_up=breakout_up,
    breakout_down=breakout_down,
    future_return_5=future_return_5
)
```

---

## Factor Analysis

### Momentum Factor Decay

**Question:** How long does momentum persist?

```python
# Current momentum
momentum_now = roc(period=20)(close)

# Future returns at various horizons
return_1d = (close[-1] - close) / close
return_5d = (close[-5] - close) / close
return_10d = (close[-10] - close) / close
return_20d = (close[-20] - close) / close

# Correlation between current momentum and future returns
corr_1d = rolling_corr(window=252)(x=momentum_now, y=return_1d)
corr_5d = rolling_corr(window=252)(x=momentum_now, y=return_5d)
corr_10d = rolling_corr(window=252)(x=momentum_now, y=return_10d)
corr_20d = rolling_corr(window=252)(x=momentum_now, y=return_20d)

# Table of decay
table_report(sql="""
    SELECT
        '1 Day' as horizon,
        AVG(corr_1d) as avg_correlation
    FROM input
    UNION ALL
    SELECT
        '5 Days' as horizon,
        AVG(corr_5d) as avg_correlation
    FROM input
    UNION ALL
    SELECT
        '10 Days' as horizon,
        AVG(corr_10d) as avg_correlation
    FROM input
    UNION ALL
    SELECT
        '20 Days' as horizon,
        AVG(corr_20d) as avg_correlation
    FROM input
""")(
    corr_1d=corr_1d,
    corr_5d=corr_5d,
    corr_10d=corr_10d,
    corr_20d=corr_20d
)
```

**Interpretation:**
- If correlation decreases with horizon: momentum mean-reverts
- If correlation stable: momentum persists
- Helps determine optimal holding period

### Value-Momentum Interaction

**Question:** Do value and momentum work together?

```python
# Value factor (from fundamentals)
ratios = financial_ratios()
pe = ratios.price_to_earnings
value_score = 1.0 / pe  # Low P/E = high value score

# Momentum factor
momentum = roc(period=60)(close)

# Future returns
return_20d = (close[-20] - close) / close

# Bucket by value and momentum
value_high = value_score > value_score[252]  # Above 1-year median
momentum_high = momentum > momentum[252]

# Combined signals
value_momentum = value_high and momentum_high  # Both
value_only = value_high and not momentum_high
momentum_only = momentum_high and not value_high
neither = not value_high and not momentum_high

# Performance by bucket
table_report(sql="""
    SELECT
        CASE
            WHEN value_momentum = 1 THEN 'Value + Momentum'
            WHEN value_only = 1 THEN 'Value Only'
            WHEN momentum_only = 1 THEN 'Momentum Only'
            WHEN neither = 1 THEN 'Neither'
        END as strategy,
        AVG(return_20d) as avg_return,
        STDDEV(return_20d) as volatility,
        COUNT(*) as count
    FROM input
    GROUP BY strategy
""")(
    value_momentum=value_momentum,
    value_only=value_only,
    momentum_only=momentum_only,
    neither=neither,
    return_20d=return_20d
)
```

---

## Calendar Effects

### Turn-of-Month Effect Research

**Question:** Does turn-of-month provide excess returns?

```python
src = market_data_source()

# Turn-of-month window
tom = turn_of_month(days_before=3, days_after=5)()
in_tom = tom.in_window

# Daily returns
daily_return = (src.c - src.c[1]) / src.c[1]

# Average returns in/out of window
table_report(sql="""
    SELECT
        CASE
            WHEN in_tom = 1 THEN 'Turn-of-Month'
            ELSE 'Other Days'
        END as period,
        AVG(daily_return) as avg_return,
        STDDEV(daily_return) as volatility,
        COUNT(*) as trading_days
    FROM input
    GROUP BY period
""")(
    in_tom=in_tom,
    daily_return=daily_return
)
```

### Day-of-Week Effects

**Question:** Which days have best/worst returns?

```python
src = market_data_source()

# Day of week
monday = day_of_week(target_day="Monday")()
tuesday = day_of_week(target_day="Tuesday")()
wednesday = day_of_week(target_day="Wednesday")()
thursday = day_of_week(target_day="Thursday")()
friday = day_of_week(target_day="Friday")()

# Daily returns
daily_return = (src.c - src.c[1]) / src.c[1]

# Aggregate by day
table_report(sql="""
    SELECT
        CASE
            WHEN monday = 1 THEN 'Monday'
            WHEN tuesday = 1 THEN 'Tuesday'
            WHEN wednesday = 1 THEN 'Wednesday'
            WHEN thursday = 1 THEN 'Thursday'
            WHEN friday = 1 THEN 'Friday'
        END as day_of_week,
        AVG(daily_return) as avg_return,
        STDDEV(daily_return) as volatility,
        COUNT(*) as count
    FROM input
    GROUP BY day_of_week
    ORDER BY avg_return DESC
""")(
    monday=monday.is_target_day,
    tuesday=tuesday.is_target_day,
    wednesday=wednesday.is_target_day,
    thursday=thursday.is_target_day,
    friday=friday.is_target_day,
    daily_return=daily_return
)
```

### Seasonal Patterns (Monthly)

**Question:** Which months outperform?

```python
src = market_data_source()

# Define all months
jan = month_of_year(target_month="January")()
feb = month_of_year(target_month="February")()
mar = month_of_year(target_month="March")()
# ... (define all 12 months)
dec = month_of_year(target_month="December")()

# Monthly returns
monthly_return = (src.c - src.c[21]) / src.c[21]  # ~21 trading days

# Aggregate by month
table_report(sql="""
    SELECT
        month_name,
        AVG(monthly_return) as avg_monthly_return,
        COUNT(*) as sample_count
    FROM input
    GROUP BY month_name
    ORDER BY avg_monthly_return DESC
""")(
    month_name=conditional_select(
        jan.is_target_month, "January",
        feb.is_target_month, "February",
        mar.is_target_month, "March",
        # ... (all months)
        "December"
    ),
    monthly_return=monthly_return
)
```

---

## Machine Learning Feature Engineering

### Creating Target Variables

**Question:** What predicts 5-day forward returns?

```python
src = market_data_source()

# Features (current)
rsi_val = rsi(period=14)(src.c)
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)
vol = volatility(period=20)(src.c)
momentum = roc(period=20)(src.c)

# Target variable (5-day forward return)
forward_return_5d = (src.c[-5] - src.c) / src.c  # Look-ahead (research only)

# Binary classification target
target_positive = forward_return_5d > 0.02  # 2% gain

# Export for ML model training
table_report(sql="""
    SELECT
        date,
        rsi_val,
        macd_line,
        vol,
        momentum,
        forward_return_5d,
        target_positive
    FROM input
    WHERE forward_return_5d IS NOT NULL
    ORDER BY date DESC
""")(
    date=timestamp,
    rsi_val=rsi_val,
    macd_line=macd_line,
    vol=vol,
    momentum=momentum,
    forward_return_5d=forward_return_5d,
    target_positive=target_positive
)
```

**⚠️ Critical:** Forward returns create look-ahead bias. Use ONLY for:
- Historical analysis
- ML model training (on past data)
- Feature importance research

**Never use in live trading!**

---

## Research Best Practices

### 1. Document Assumptions

```python
# GOOD: Clear assumptions
# Assumption: Gaps > 1.5% are significant for SPY
# Assumption: 100% fill = gap completely closed
gaps = session_gap(fill_percent=100, timeframe="1Min")()
```

### 2. Test Multiple Parameters

```python
# Test different thresholds
gap_small = session_gap(fill_percent=100, min_gap_size=0.5)()
gap_medium = session_gap(fill_percent=100, min_gap_size=1.0)()
gap_large = session_gap(fill_percent=100, min_gap_size=2.0)()

# Compare fill rates
```

### 3. Check Sample Size

```python
table_report(sql="""
    SELECT
        COUNT(*) as total_gaps,
        SUM(CASE WHEN gap_filled = 1 THEN 1 ELSE 0 END) as filled_count,
        AVG(gap_size) as avg_size
    FROM input
    WHERE gap_size > 1.0
""")( ... )
```

### 4. Look for Robustness

- Test across multiple assets
- Test across different time periods
- Test with different parameters
- Check statistical significance

### 5. Avoid Overfitting

- Simple models generalize better
- More parameters = more overfitting risk
- In-sample vs out-of-sample testing

---

## Research Dashboard Workflow

Research scripts generate interactive dashboards with multiple report types. Understanding the dashboard workflow is critical for effective analysis.

### Dashboard Components

When you run a research script with reporting transforms, the results appear in a dedicated **Research Dashboard** (separate from live trading charts):

**Available Report Types:**
1. **Gap Reports** - Comprehensive gap analysis with cards, charts, tables
2. **Table Reports** - SQL-based tabular analysis
3. **Bar Charts** - Category comparisons
4. **Pie Charts** - Proportional breakdowns
5. **Histograms** - Distribution analysis
6. **Line Charts** - Time series trends
7. **Card Reports** - Key metrics summary

### Typical Dashboard Workflow

#### Step 1: Run Research Script

```python
# Gap analysis research script
gaps = session_gap(fill_percent=100, timeframe="1Min")()

gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)
```

#### Step 2: Review Dashboard Outputs

The dashboard organizes results into tabs/sections:

**Gap Report Tab:**
- **Cards Section**: Fill rate %, average gap size, median fill time
- **Histogram Section**: Gap size distribution, fill time distribution
- **Table Section**: Gaps by hour, gaps by weekday, gaps by size bucket

#### Step 3: Interactive Analysis

**Within the Dashboard:**
- Click table rows to filter charts
- Hover over histograms for exact counts
- Export tables to CSV for external analysis
- Compare across different parameter sets

#### Step 4: Iterative Refinement

Based on dashboard insights, refine your research:

```python
# Initial research shows gaps > 2.0% rarely fill
# Refine to focus on smaller gaps
gaps = session_gap(fill_percent=100, timeframe="1Min")()

# Add table report to segment by size
table_report(sql="""
    SELECT
        CASE
            WHEN gap_size < 0.5 THEN 'Micro'
            WHEN gap_size < 1.0 THEN 'Small'
            WHEN gap_size < 2.0 THEN 'Medium'
            ELSE 'Large'
        END as size_category,
        COUNT(*) as count,
        AVG(CASE WHEN gap_filled = 1 THEN 1.0 ELSE 0.0 END) as fill_rate,
        AVG(fill_time_minutes) as avg_fill_time
    FROM input
    GROUP BY size_category
    ORDER BY gap_size
""")(
    gap_size=gaps.gap_size,
    gap_filled=gaps.gap_filled,
    fill_time_minutes=gaps.fill_time_minutes
)
```

### Multi-Report Research Example

Combine multiple report types for comprehensive analysis:

```python
# Price action pattern research
src = market_data_source()

# Detect hammers and shooting stars
hammer = hammer_pattern()(src.o, src.h, src.l, src.c)
shooting_star = shooting_star_pattern()(src.o, src.h, src.l, src.c)

# Calculate forward returns (research only!)
fwd_1d = forward_returns(period=1)(src.c)
fwd_5d = forward_returns(period=5)(src.c)

# Card Report: Overall statistics
numeric_card_report()(
    hammer_count=agg_sum()(hammer),
    shooting_star_count=agg_sum()(shooting_star),
    avg_1d_return=agg_mean()(fwd_1d),
    avg_5d_return=agg_mean()(fwd_5d)
)

# Bar Chart: Win rate by pattern
bar_chart_report(x_axis_column="pattern")(
    pattern=pattern_name,
    win_rate=win_rate_pct
)

# Table Report: Detailed breakdowns
table_report(sql="""
    SELECT
        CASE
            WHEN hammer = 1 THEN 'Hammer'
            WHEN shooting_star = 1 THEN 'Shooting Star'
            ELSE 'No Pattern'
        END as pattern,
        COUNT(*) as occurrences,
        AVG(fwd_1d) as avg_1d_return,
        AVG(fwd_5d) as avg_5d_return,
        STDDEV(fwd_1d) as volatility_1d,
        SUM(CASE WHEN fwd_1d > 0 THEN 1 ELSE 0 END) * 100.0 / COUNT(*) as win_rate_1d
    FROM input
    WHERE hammer = 1 OR shooting_star = 1
    GROUP BY pattern
""")(
    hammer=hammer,
    shooting_star=shooting_star,
    fwd_1d=fwd_1d,
    fwd_5d=fwd_5d
)

# Histogram: Return distribution
histogram_chart_report(bins=20, x_axis_column="returns")(
    returns=fwd_1d
)
```

**Dashboard Output:**
1. **Cards**: Total patterns detected, average returns
2. **Bar Chart**: Visual comparison of win rates
3. **Table**: Detailed statistics with statistical significance
4. **Histogram**: Return distribution (look for skew, outliers)

### Research to Strategy Conversion

Once research validates an edge, convert to trading strategy:

**Research Script (uses forward_returns):**
```python
# Research: Does RSI < 30 predict positive returns?
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30
fwd_returns = forward_returns(period=5)(src.c)  # Look-ahead!

table_report(sql="""
    SELECT
        AVG(fwd_returns) as avg_return,
        STDDEV(fwd_returns) as volatility,
        COUNT(*) as sample_size
    FROM input
    WHERE oversold = 1
""")(oversold=oversold, fwd_returns=fwd_returns)
```

**Trading Strategy (no look-ahead):**
```python
# Convert to live trading (NO forward_returns)
rsi_val = rsi(period=14)(src.c)
buy = rsi_val < 30
exit_profit = rsi_val > 70

trade_signal_executor()(
    enter_long=buy,
    exit_long=exit_profit
)
```

### Dashboard Best Practices

#### 1. Start with Overview Cards

```python
# High-level metrics first
numeric_card_report()(
    total_signals=agg_sum()(signal),
    avg_return=agg_mean()(returns),
    win_rate=agg_mean()(profitable)
)
```

#### 2. Add Charts for Visualization

```python
# Visual patterns
bar_chart_report(x_axis_column="day_of_week")(
    day_of_week=dow,
    avg_return=returns_by_day
)
```

#### 3. Use Tables for Detailed Breakdowns

```python
# Granular analysis
table_report(sql="""
    SELECT hour, COUNT(*), AVG(return)
    FROM input
    GROUP BY hour
    ORDER BY hour
""")(hour=hour, return=ret)
```

#### 4. Histograms for Distributions

```python
# Understand distribution shape
histogram_chart_report(bins=30)(
    values=return_distribution
)
```

### Common Dashboard Patterns

#### Pattern 1: Segmentation Analysis

```python
# Analyze different market regimes
regime = regime_detector()(...)

table_report(sql="""
    SELECT
        regime,
        COUNT(*) as samples,
        AVG(signal_return) as avg_return,
        SUM(CASE WHEN signal_return > 0 THEN 1 ELSE 0 END) * 100.0 / COUNT(*) as win_rate
    FROM input
    GROUP BY regime
""")(regime=regime, signal_return=fwd_returns)
```

#### Pattern 2: Time-Based Analysis

```python
# Calendar effect research
dow = day_of_week()()
hour = hour_of_day()()

table_report(sql="""
    SELECT
        dow,
        hour,
        AVG(returns) as avg_return,
        COUNT(*) as count
    FROM input
    GROUP BY dow, hour
    ORDER BY dow, hour
""")(dow=dow, hour=hour, returns=returns)
```

#### Pattern 3: Parameter Sensitivity

```python
# Test multiple parameter combinations (run separately)
# Script 1: RSI 10
rsi_10 = rsi(period=10)(src.c)
# ...generate reports

# Script 2: RSI 14
rsi_14 = rsi(period=14)(src.c)
# ...generate reports

# Script 3: RSI 20
rsi_20 = rsi(period=20)(src.c)
# ...generate reports

# Compare dashboards side-by-side
```

### Dashboard Limitations

**What Dashboard CANNOT Do:**
1. **No live updating**: Dashboard shows historical research results (not real-time)
2. **No parameter sweeps**: Must run separate scripts for different parameters
3. **No optimization**: Dashboard is for analysis, not automated parameter finding
4. **No backtesting**: Use trading strategies with `trade_signal_executor()` for backtests

**What Dashboard CAN Do:**
1. **Interactive exploration**: Click, filter, drill down
2. **Multiple views**: Cards, charts, tables for different perspectives
3. **Export results**: Save tables, charts for presentations
4. **Comparative analysis**: Run multiple scripts, compare dashboards

### Integration with Strategy Development

**Research → Strategy Workflow:**

1. **Hypothesis**: "Do session gaps larger than 1% fill within 2 hours?"
2. **Research Script**: Use `gap_report()` to analyze historical gap behavior
3. **Dashboard Analysis**: Review fill rates, timing, size distribution
4. **Insight**: "Gaps 0.5-1.5% fill 78% of the time within 90 minutes"
5. **Strategy Design**: Build gap fade strategy with size filter
6. **Trading Strategy**: Implement with `trade_signal_executor()`
7. **Backtest**: Evaluate performance metrics
8. **Refinement**: Return to research if results don't match expectations

---

## Summary

### Research Script Components

1. **No Executor**: Research scripts don't call `trade_signal_executor()`
2. **Reporting Sinks**: Use `gap_report()`, `table_report()`, card reports, charts
3. **Look-Ahead Allowed**: Can use `forward_returns()` for historical analysis (NOT for trading)
4. **SQL Queries**: `table_report()` supports complex aggregations
5. **Statistical Analysis**: Focus on understanding, not trading
6. **Dashboard Output**: Results appear in interactive research dashboard (separate from trading charts)

### Common Research Questions

- **Gap Analysis**: Fill rates, timing, size distribution
- **Correlations**: Lead-lag, stability, regime shifts
- **Patterns**: Frequency, success rate, directional bias
- **Factors**: Persistence, decay, interactions
- **Calendar**: Day-of-week, month, turn-of-month effects
- **ML Features**: Predictive power, target engineering

### Key Difference

**Strategy:** What should I trade?
**Research:** Why does this work? How robust is it?

---

**Next:** [Advanced Topics →](06-advanced-topics.md)
