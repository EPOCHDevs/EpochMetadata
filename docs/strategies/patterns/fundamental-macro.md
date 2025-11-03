---
page_type: implementation
layout: default
order: 6
category: Strategies
description: Fundamental and macro strategies including earnings plays, value momentum, and calendar effects
parent: ./index.md
---

# Fundamental & Macro

:::note
For detailed fundamental data reference (balance sheet, income statement, cash flow, insider trading, 13F holdings), see [Fundamental Reference](../fundamental-reference.md)
:::

## Earnings Momentum

**Strategy:** Buy stocks with positive earnings surprises and momentum

```epochscript
src = market_data_source()

# Fundamental data
income = income_statement()
eps = income.basic_earnings_per_share

# Earnings growth (QoQ)
eps_growth = (eps - eps[1]) / eps[1]

# Price momentum
returns_20 = roc(period=20)(src.c)

# Combined signal
strong_earnings = eps_growth > 0.15  # 15% earnings growth
strong_momentum = returns_20 > 5.0   # 5% price momentum

entry = strong_earnings and strong_momentum

trade_signal_executor()(enter_long=entry)
```

### Value + Momentum Combo

**Strategy:** Buy undervalued stocks with positive momentum

```epochscript
src = market_data_source()

# Value metrics
ratios = financial_ratios()
pe = ratios.price_to_earnings
pb = ratios.price_to_book

# Value signal (low P/E and P/B)
undervalued = (pe < 15) and (pb < 2.0)

# Momentum signal
momentum = roc(period=60)(src.c)
positive_momentum = momentum > 0

# Quality filter
roe = ratios.return_on_equity
high_quality = roe > 15.0

# Combined signal
entry = undervalued and positive_momentum and high_quality

trade_signal_executor()(enter_long=entry)
```

### Economic Surprise Strategy

**Strategy:** Trade based on economic indicator surprises

```epochscript
# Economic data (FRED)
gdp = economic_indicator(category="GDP")()
unemployment = economic_indicator(category="Unemployment")()
cpi = economic_indicator(category="CPI")()

# Calculate surprise (actual vs expectation)
# Expectation = moving average
gdp_ma = sma(period=4)(gdp)  # 4 quarters
gdp_surprise = (gdp - gdp_ma) / gdp_ma

# Positive surprise (growth accelerating)
positive_surprise = gdp_surprise > 0.01  # 1% above trend

# Market data
src = market_data_source()

# Trend following on positive surprise
fast = ema(period=20)(src.c)
slow = ema(period=50)(src.c)
trend_up = fast > slow

entry = positive_surprise and trend_up

trade_signal_executor()(enter_long=entry)
```

:::note
For comprehensive calendar effects (Holiday Effect, Month/Day/Week patterns, Turn of Month scoring), see [Calendrical Effects](../calendrical-effects.md)
:::

### Turn-of-Month Effect

**Strategy:** Exploit monthly equity inflows (institutional rebalancing)

```epochscript
src = market_data_source()

# Turn-of-month window (3 days before, 5 days after)
tom = turn_of_month(days_before=3, days_after=5)()
in_window = tom.result

# Bullish bias during window
trend = ema(period=50)(src.c)
above_trend = src.c > trend

# Enter on turn-of-month + uptrend
entry = in_window and above_trend

# Exit after window closes
exit = not in_window

trade_signal_executor()(
    enter_long=entry,
    exit_long=exit
)
```

### January Effect (Small-Cap)

**Strategy:** Overweight small-caps in January

```epochscript
src = market_data_source()

# January detection
january = month_of_year(target_month="January")()
is_january = january.is_target_month

# Small-cap momentum (cross-sectional)
returns = roc(period=20)(src.c)
momentum = cs_momentum()(returns)

# Stronger filter in January (top 20 vs top 10)
entry = conditional_select(
    is_january, top_k(k=20)(momentum),  # More aggressive in Jan
    top_k(k=10)(momentum)               # Normal rest of year
)

trade_signal_executor()(enter_long=entry)
```

---
