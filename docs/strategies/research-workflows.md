---
page_type: concept
layout: default
order: 1
category: Strategies
description: Market research and analysis workflows using EpochScript for non-trading analysis
parent: ./index.md
---

# Research Workflows

Market research and analysis using Epoch script (non-trading scripts).

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

```epochscript
# MUST have trade_signal_executor
src = market_data_source()
signal = rsi(period=14)(src.c) < 30
trade_signal_executor()(enter_long=signal)  # Required
```

### Research Script Example

```epochscript
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

Research gap fill behavior across different markets and sessions.

```epochscript
# Overnight gaps - session boundaries
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled, gaps.gap_retrace, gaps.gap_size,
    gaps.psc, gaps.psc_timestamp
)

# Intraday gaps - trading halts (2% threshold for stocks)
intraday_gaps = bar_gap(fill_percent=100, min_gap_size=2.0, timeframe="1Min")()

# FX gaps - small moves (4 pips = 0.04%)
fx_gaps = bar_gap(fill_percent=100, min_gap_size=0.04, timeframe="1Min")()

# Session-specific gaps (compare London vs NY vs Asian)
london_gaps = session_gap(fill_percent=100, timeframe="1Min", session="London")()
```

**Report Output:**
- **Cards**: Fill rate %, average size, median fill time
- **Histograms**: Gap size distribution, fill time distribution
- **Tables**: By hour, size bucket, day of week

**Research Questions:** Fill rate by gap size? Time-to-fill patterns? Session variations? Day-of-week effects?

---

## Correlation Studies

Research lead-lag relationships, stability, and cross-asset dynamics.

```epochscript
# Lead-Lag Analysis: Does crypto volatility predict tech drawdowns?
crypto_vol = volatility(period=20)(crypto.c)
tech_dd = ulcer_index(period=20)(tech.c)
concurrent_corr = rolling_corr(window=20)(x=crypto_vol, y=tech_dd)
lag5_corr = rolling_corr(window=20)(x=crypto_vol[5], y=tech_dd)  # 5-day lag

# Stability: Short vs long window (regime shifts when divergence > 0.3)
corr_short = rolling_corr(window=20)(x=asset1.c, y=asset2.c)
corr_long = rolling_corr(window=60)(x=asset1.c, y=asset2.c)
regime_shift = abs(corr_short - corr_long) > 0.3

# Cross-Asset Matrix: Pairwise correlations for diversification analysis
corr_ab = rolling_corr(window=60)(x=asset_a.c, y=asset_b.c)
corr_ac = rolling_corr(window=60)(x=asset_a.c, y=asset_c.c)
avg_corr = (corr_ab + corr_ac) / 2.0  # Diversification measure

# Table report for time series analysis
table_report(sql="SELECT date, concurrent_corr, lag5_corr FROM input ORDER BY date DESC LIMIT 100")(
    date=timestamp, concurrent_corr=concurrent_corr, lag5_corr=lag5_corr
)
```

**Analysis:** Compare lag correlations (if lag5_corr > concurrent_corr → leading indicator). Track regime shifts (correlation divergence). Monitor diversification (lower avg_corr = better).

---

## Pattern Research

Analyze pattern frequency, success rates, and directional bias.

```epochscript
# Head & Shoulders: Occurrence and target hit rate
hs = head_and_shoulders(tolerance=0.02)()
breakdown = src.c < hs.neckline
target_hit = src.c <= hs.target

# Triangle Breakouts: Pattern type vs direction vs returns
tri = triangles(min_pattern_bars=15)()
breakout_up = tri.breakout and (src.c > tri.upper_line)
future_return_5 = forward_returns(period=5)(src.c)  # Research only

# Table analysis
table_report(sql="""
    SELECT COUNT(*) as count, AVG(target_hit) as success_rate
    FROM input WHERE pattern_detected = 1
""")(pattern_detected=hs.pattern_detected, target_hit=target_hit)
```

---

## Factor Analysis

### Momentum Factor Decay

**Question:** How long does momentum persist?

```epochscript
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

```epochscript
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

```epochscript
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

```epochscript
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

```epochscript
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

```epochscript
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

**Note: Critical:** Forward returns create look-ahead bias. Use ONLY for:
- Historical analysis
- ML model training (on past data)
- Feature importance research

**Never use in live trading!**

---

## Research Best Practices

### 1. Document Assumptions

```epochscript
# GOOD: Clear assumptions
# Assumption: Gaps > 1.5% are significant for SPY
# Assumption: 100% fill = gap completely closed
gaps = session_gap(fill_percent=100, timeframe="1Min")()
```

### 2. Test Multiple Parameters

```epochscript
# Test different thresholds
gap_small = session_gap(fill_percent=100, min_gap_size=0.5)()
gap_medium = session_gap(fill_percent=100, min_gap_size=1.0)()
gap_large = session_gap(fill_percent=100, min_gap_size=2.0)()

# Compare fill rates
```

### 3. Check Sample Size

```epochscript
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

Research scripts generate interactive dashboards with multiple report types for comprehensive analysis.

### Dashboard Components

Results appear in a dedicated **Research Dashboard** (separate from live trading charts):

**Available Report Types:**
1. **Gap Reports** - Comprehensive gap analysis (cards, charts, tables)
2. **Table Reports** - SQL-based tabular analysis
3. **Bar/Pie/Histogram/Line Charts** - Visual analysis
4. **Card Reports** - Key metrics summary

### Workflow: Run → Review → Refine

```epochscript
# 1. Run research script
gaps = session_gap(fill_percent=100, timeframe="1Min")()
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled, gaps.gap_retrace, gaps.gap_size,
    gaps.psc, gaps.psc_timestamp
)

# 2. Dashboard shows: Cards (fill rate %), Histograms (distribution), Tables (by hour/size)
# 3. Interactive: Click rows to filter, hover for details, export CSV
# 4. Refine based on insights (e.g., gaps > 2% rarely fill → focus on smaller gaps)
```

### Multi-Report Analysis

Combine report types for comprehensive research:

```epochscript
# Pattern research with multiple outputs
src = market_data_source()
hammer = hammer_pattern()(src.o, src.h, src.l, src.c)
fwd_1d = forward_returns(period=1)(src.c)  # Research only!

# Card: Summary stats
numeric_card_report()(
    hammer_count=agg_sum()(hammer),
    avg_return=agg_mean()(fwd_1d)
)

# Table: Detailed breakdown with SQL
table_report(sql="""
    SELECT COUNT(*) as occurrences, AVG(fwd_1d) as avg_return,
           SUM(CASE WHEN fwd_1d > 0 THEN 1 ELSE 0 END) * 100.0 / COUNT(*) as win_rate
    FROM input WHERE hammer = 1
""")(hammer=hammer, fwd_1d=fwd_1d)

# Histogram: Return distribution
histogram_chart_report(bins=20)(returns=fwd_1d)
```

### Research to Strategy Conversion

```epochscript
# Research (uses forward_returns)
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30
fwd_returns = forward_returns(period=5)(src.c)  # Look-ahead allowed
table_report(sql="SELECT AVG(fwd_returns), COUNT(*) FROM input WHERE oversold = 1")(
    oversold=oversold, fwd_returns=fwd_returns
)

# Convert to Trading Strategy (NO look-ahead)
rsi_val = rsi(period=14)(src.c)
buy = rsi_val < 30
trade_signal_executor()(enter_long=buy, exit_long=(rsi_val > 70))
```

### Dashboard Best Practices

**Layered Analysis Approach:**
1. **Cards** - High-level metrics (total signals, win rate, avg return)
2. **Charts** - Visual patterns (bar charts for time-based, histograms for distributions)
3. **Tables** - Detailed breakdowns with SQL (segmentation, grouping, filtering)

**Common Patterns:**
```epochscript
# Segmentation: Analyze by market regime
table_report(sql="""
    SELECT regime, COUNT(*), AVG(return),
           SUM(CASE WHEN return > 0 THEN 1 ELSE 0 END) * 100.0 / COUNT(*) as win_rate
    FROM input GROUP BY regime
""")(regime=regime, return=fwd_returns)

# Time-based: Calendar effects
table_report(sql="""
    SELECT dow, hour, AVG(returns), COUNT(*)
    FROM input GROUP BY dow, hour ORDER BY dow, hour
""")(dow=day_of_week()(), hour=hour_of_day()(), returns=returns)
```

### Dashboard Capabilities

**CAN:** Interactive exploration, multiple views (cards/charts/tables), export results, comparative analysis
**CANNOT:** Live updating, automated parameter sweeps, optimization, real-time backtesting

### Research → Strategy Workflow

1. Hypothesis → 2. Research Script → 3. Dashboard Analysis → 4. Extract Insights → 5. Strategy Design → 6. Implement with `trade_signal_executor()` → 7. Backtest → 8. Refine

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

**Next:** [Techniques →](./techniques.md) | [Guidelines & Best Practices →](./guidelines.md)
