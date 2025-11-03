---
page_type: implementation
layout: default
order: 3
category: Concepts
description: Research reports for gap analysis, table queries, and chart visualizations
parent: ./visualization.md
---

# Research Reports

## Overview

**Generate research dashboards** to analyze strategy behavior:

**Workflow:**
1. Write research script with report transforms
2. Run script → Generate reports
3. Open dashboard → View results
4. Analyze findings → Refine strategy

---

## Gap Reports

Comprehensive gap analysis with summary cards, histograms, and detailed tables.

### Required Inputs

The `gap_report` transform requires **all 5 outputs** from `session_gap` or `bar_gap`:

```epochscript
# Get gap detection outputs
gaps = session_gap(fill_percent=100, timeframe="1Min")()

# Pass all 5 required outputs to gap_report
gap_report(fill_time_pivot_hour=12, histogram_bins=15)(
    gaps.gap_filled,      # Boolean: Was gap filled?
    gaps.gap_retrace,     # Boolean: Did price retrace into gap?
    gaps.gap_size,        # Decimal: Gap size in price units
    gaps.psc,             # Decimal: Previous session close
    gaps.psc_timestamp    # Timestamp: When gap formed
)
```

### Dashboard Components

**Summary Cards:**
- Fill Rate: 67.3%
- Average Gap Size: 1.24%
- Median Fill Time: 42 minutes
- Total Gaps: 1,247

**Histograms:**
- Gap Size Distribution (bins: 0-0.5%, 0.5-1%, 1-1.5%, etc.)
- Fill Time Distribution (bins: 0-15min, 15-30min, 30-60min, etc.)

**Tables:**
- Gaps by Hour (pivot table)
- Gaps by Day of Week
- Gaps by Size Bucket
- Detailed gap log (timestamp, size, fill time, filled?)

### Common Patterns

**Analyze gaps by session:**

```epochscript
# Asian session gaps
asian_gaps = session_gap(fill_percent=100, session="AsianKillZone")()
gap_report(fill_time_pivot_hour=12)(
    asian_gaps.gap_filled,
    asian_gaps.gap_retrace,
    asian_gaps.gap_size,
    asian_gaps.psc,
    asian_gaps.psc_timestamp
)

# London session gaps
london_gaps = session_gap(fill_percent=100, session="LondonKillZone")()
gap_report(fill_time_pivot_hour=12)(
    london_gaps.gap_filled,
    london_gaps.gap_retrace,
    london_gaps.gap_size,
    london_gaps.psc,
    london_gaps.psc_timestamp
)
```

---

## Table Reports

SQL-based data analysis and aggregation.

### Basic Usage

```epochscript
# Calculate returns by RSI regime
returns = (src.c - src.c[1]) / src.c[1]
rsi_val = rsi(period=14)(src.c)
volume = src.v

table_report(sql="""
    SELECT
        CASE
            WHEN SLOT0 < 30 THEN 'Oversold'
            WHEN SLOT0 > 70 THEN 'Overbought'
            ELSE 'Neutral'
        END as regime,
        AVG(SLOT1) as avg_return,
        STDDEV(SLOT1) as volatility,
        AVG(SLOT2) as avg_volume,
        COUNT(*) as count
    FROM self
    GROUP BY regime
    ORDER BY avg_return DESC
""")(
    rsi_val,    # SLOT0
    returns,    # SLOT1
    volume      # SLOT2
)
```

**Dashboard table:**
```text
┌─────────────┬─────────────┬────────────┬─────────────┬───────┐
│ Regime      │ Avg Return  │ Volatility │ Avg Volume  │ Count │
├─────────────┼─────────────┼────────────┼─────────────┼───────┤
│ Oversold    │ +0.42%      │ 1.85%      │ 3.2M        │ 347   │
│ Neutral     │ +0.05%      │ 1.12%      │ 2.1M        │ 8,234 │
│ Overbought  │ -0.18%      │ 1.67%      │ 2.8M        │ 412   │
└─────────────┴─────────────┴────────────┴─────────────┴───────┘
```

### SQL Column Mapping

**Inputs are accessed as SLOT0, SLOT1, SLOT2, etc. in order:**

```epochscript
# Inputs map to SLOT columns by position
returns = (src.c - src.c[1]) / src.c[1]
rsi_val = rsi(period=14)(src.c)

table_report(sql="""
    SELECT AVG(SLOT0), AVG(SLOT1)
    FROM self
""")(
    returns,    # SLOT0
    rsi_val     # SLOT1
)
```

:::warning
**Use SLOT* column names**

SQL must reference inputs as SLOT0, SLOT1, SLOT2, etc.:
```epochscript
src = market_data_source()
returns = (src.c - src.c[1]) / src.c[1]

# ❌ WRONG: Using variable names
table_report(sql="SELECT AVG(returns) FROM self")(returns)

# ✅ CORRECT: Use SLOT0
table_report(sql="SELECT AVG(SLOT0) FROM self")(returns)
```
:::

### Common SQL Patterns

**Aggregation by group:**

```epochscript
src = market_data_source()
returns = (src.c - src.c[1]) / src.c[1]
rsi_val = rsi(period=14)(src.c)
oversold = rsi_val < 30
overbought = rsi_val > 70
regime_label = conditional_select(
    oversold, "Oversold",
    overbought, "Overbought",
    "Neutral"
)

table_report(sql="""
    SELECT
        SLOT0 as regime,
        AVG(SLOT1) as avg_return,
        COUNT(*) as count
    FROM self
    GROUP BY SLOT0
""")(regime_label, returns)
```

**Time-based analysis:**

```epochscript
table_report(sql="""
    SELECT
        EXTRACT(HOUR FROM SLOT0) as hour,
        AVG(SLOT1) as avg_volume
    FROM self
    GROUP BY hour
    ORDER BY hour
""")(src.t, src.v)
```

**Conditional aggregation:**

```epochscript
table_report(sql="""
    SELECT
        CASE
            WHEN SLOT0 < 30 THEN 'Oversold'
            WHEN SLOT0 > 70 THEN 'Overbought'
            ELSE 'Neutral'
        END as condition,
        AVG(SLOT1) as performance
    FROM self
    GROUP BY condition
""")(rsi_val, returns)
```

:::tip
**SQL Syntax**: Uses DuckDB SQL with support for:
- Aggregations: `AVG()`, `SUM()`, `COUNT()`, `MIN()`, `MAX()`, `STDDEV()`
- Window functions: `ROW_NUMBER()`, `RANK()`, `LAG()`, `LEAD()`
- Date functions: `EXTRACT()`, `DATE_TRUNC()`
- String functions: `CONCAT()`, `SUBSTRING()`, `UPPER()`, `LOWER()`
:::

---

## Chart Reports

Visual reports for distribution and relationship analysis.

### Histogram Reports

**Distribution visualization:**

```epochscript
# Return distribution
returns = (src.c - src.c[1]) / src.c[1]
histogram_chart_report(bins=20, title="Return Distribution")(
    returns=returns
)
```

**Parameters:**
- `bins`: Number of histogram bins
- `title`: Chart title

### Bar Chart Reports

**Categorical comparisons:**

```epochscript
# Count by regime
rsi_val = rsi(period=14)(src.c)
regime = conditional_select(
    rsi_val < 30, "Oversold",
    rsi_val > 70, "Overbought",
    "Neutral"
)

bar_chart_report(x="regime", y="count", title="Bars by Regime")(
    regime=regime
)
```

**Parameters:**
- `x`: X-axis column name
- `y`: Y-axis aggregation (count, sum, avg, etc.)
- `title`: Chart title

### Pie Chart Reports

**Proportion visualization:**

```epochscript
# Market regime proportions
pie_chart_report(category="regime", value="count", title="Regime Distribution")(
    regime=regime
)
```

**Parameters:**
- `category`: Column for pie segments
- `value`: Aggregation method
- `title`: Chart title

### Line Chart Reports

**Time series and trends:**

```epochscript
# Average volume by hour
hour = extract_hour(src.t)
volume = src.v

lines_chart_report(x="hour", y="volume", title="Hourly Volume Pattern")(
    hour=hour,
    volume=volume
)
```

**Parameters:**
- `x`: X-axis column (typically time or sequence)
- `y`: Y-axis value
- `title`: Chart title

### Scatter Plot Reports

**Relationship analysis:**

```epochscript
# RSI vs Returns
rsi_val = rsi(period=14)(src.c)
returns = (src.c - src.c[1]) / src.c[1]

scatter_chart_report(x="rsi_val", y="returns", title="RSI vs Returns")(
    rsi_val=rsi_val,
    returns=returns
)
```

**Parameters:**
- `x`: X-axis variable
- `y`: Y-axis variable
- `title`: Chart title

---

## Multiple Reports

**Combine multiple reports** in a single research script:

```epochscript
src = market_data_source()
gaps = session_gap(fill_percent=100, timeframe="1Min")()
rsi_val = rsi(period=14)(src.c)
returns = (src.c - src.c[1]) / src.c[1]

# Gap analysis
gap_report(fill_time_pivot_hour=12)(
    gaps.gap_filled,
    gaps.gap_retrace,
    gaps.gap_size,
    gaps.psc,
    gaps.psc_timestamp
)

# RSI regime analysis
table_report(sql="""
    SELECT
        CASE WHEN SLOT0 < 30 THEN 'Oversold'
             WHEN SLOT0 > 70 THEN 'Overbought'
             ELSE 'Neutral'
        END as regime,
        AVG(SLOT1) as avg_return,
        COUNT(*) as count
    FROM self
    GROUP BY regime
""")(rsi_val, returns)

# Return distribution (commented out - histogram_chart_report requires 'category' option)
# histogram_chart_report(bins=30, title="Return Distribution")(
#     returns=returns
# )
```

All reports appear in the dashboard with separate sections.

---

**Previous:** [Core Features ←](./core-features.md)
