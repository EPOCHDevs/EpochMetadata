---
page_type: concept
layout: default
order: 3
category: Core Concepts
description: Multi-asset ranking and selection for factor strategies
parent: ./index.md
---

# Cross-Sectional Analysis

Multi-asset ranking and selection. Evaluate multiple assets simultaneously to generate rankings, factors, or universe filters.

---

## Requirements

1. **Multiple assets:** Minimum 10+ recommended
2. **Same timestamp:** All assets evaluated at the same time point
3. **Universe specification:** External configuration (not in script)

---

## Cross-Sectional Transforms

```epochscript
# Momentum ranking across universe
returns = roc(period=20)(close)
momentum = cs_momentum()(returns)

# Select top K assets
top10 = top_k(k=10)(momentum)
top20_percent = top_k_percent(k=20)(momentum)

# Select bottom K assets
bottom10 = bottom_k(k=10)(momentum)

# Trade top performers
trade_signal_executor()(enter_long=top10)
```

---

## Available Transforms

| Transform | Description |
|-----------|-------------|
| `cs_momentum` | Rank assets by momentum (percentile) |
| `top_k` | Select top K assets |
| `bottom_k` | Select bottom K assets |
| `top_k_percent` | Select top K% of assets |
| `bottom_k_percent` | Select bottom K% of assets |

---

## Rules

1. **Cannot mix single-asset and multi-asset strategies**
2. **Requires external universe definition** (not in script)
3. **All transforms must support cross-sectional mode**
4. **Rankings are relative within the universe**

---

## Factor Strategy Example

```epochscript
# Calculate multiple factors
momentum = roc(period=20)(close)
value = earnings_yield()  # From fundamental data
quality = roe()

# Combine factors (equal-weight)
combined = (momentum + value + quality) / 3

# Select top 20 stocks
top_stocks = top_k(k=20)(combined)

trade_signal_executor()(enter_long=top_stocks)
```

---

## See Also

- [Design Guidelines](../design-guidelines.md) - Cross-sectional constraints
